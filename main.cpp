#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <fcntl.h>
#include <sys/epoll.h>
#include <csignal>
#include "locker.hpp"
#include "thread_poll.hpp"
#include "http_connection.h"

#define MAX_FD 65536 // 最大的文件描述符个数
#define MAX_EVENT_NUMBER 10000 // 监听的最大的事件数量

// 添加文件描述符到epoll对象中，并设置是否采用one_shot
extern void AddFd(int epoll_fd, int fd, bool one_shot);
// 从epoll对象中删除文件描述符
extern void RemoveFd(int epoll_fd, int fd);


// 添加信号捕捉
void AddSig(int sig, void(handler)(int))
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigfillset(&sa.sa_mask); // 处理sig过程中阻塞所有信号
    sa.sa_handler = handler;
    if (sigaction(sig, &sa, NULL) == -1)
    {
        throw std::exception();
    }
}

int main(int argc, char *argv[])
{
    if (argc <= 1)
    {
        printf("按照如下格式运行：%s port_number\n", basename(argv[0]));
        exit(-1);
    }

    // 设置信号捕捉，捕捉 SIGPIPE 信号，它在管道一端断开连接后，另一端仍在读或写时出现，默认终止进程，设置为忽略 
    AddSig(SIGPIPE, SIG_IGN);

    // 创建线程池
    ThreadPool<HttpConnection> *pool = NULL;
    try
    {
        pool = new ThreadPool<HttpConnection>();
    }
    catch(...)
    {
        return 1;
    }
    
    // 创建用户数组
    HttpConnection *users = new HttpConnection[MAX_FD];

    // 创建套接字
    int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1)
    {
        perror("socket");
        exit(-1);
    }

    // 端口复用
    int reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // 绑定，监听
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));
    if (bind(listen_fd, (struct sockaddr *)(&server_addr), sizeof(server_addr)) == -1)
    {
        perror("bind");
        exit(-1);
    }
    if (listen(listen_fd, 5) == -1)
    {
        perror("listen");
        exit(-1);
    }

    // 创建epool对象并设置监听，创建事件数组
    int epoll_fd = epoll_create(5);
    if (epoll_fd == -1)
    {
        perror("epoll_create");
        exit(-1);
    }
    HttpConnection::m_epoll_fd = epoll_fd;
    AddFd(epoll_fd, listen_fd, false);
    epoll_event events[MAX_EVENT_NUMBER];

    // 处理连接请求
    while (true)
    {
        int number = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1); // 阻塞
        if (number < 0 && errno != EINTR)
        {
            printf("epoll failure\n");
            break;
        }
        for (int i = 0; i < number; ++i)
        {
            int sock_fd = events[i].data.fd;
            if (sock_fd == listen_fd)
            {
                struct sockaddr_in client_addr;
                socklen_t client_addr_size = sizeof(client_addr);
                int new_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_size);
                if (new_fd == -1)
                {
                    perror("accept");
                    continue;
                }
                if (HttpConnection::m_connection_number >= MAX_FD)
                {
                    // 可以返回信息说服务器正忙
                    close(new_fd);
                    continue;
                }
                // 初始化信息，fd从小到大给的，直接作为下标，不会冲突
                users[new_fd].Initialization(new_fd, client_addr);
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                users[sock_fd].CloseConnection();
            }
            else if (events[i].events & EPOLLIN)
            {
                if (users[sock_fd].Read())
                {
                    pool->append(users + sock_fd);
                }
                else
                {
                    users[sock_fd].CloseConnection();
                }
            } else if (events[i].events & EPOLLOUT)
            {
                if (!users[sock_fd].Write())
                {
                    users[sock_fd].CloseConnection();
                }
            }
        }
    }

    close(epoll_fd);
    close(listen_fd);
    delete []users;
    delete pool;
    return 0;
}