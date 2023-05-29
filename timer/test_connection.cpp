#include <stdlib.h>
#include <cassert>
#include <exception>
#include <sys/epoll.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include "timer.h"

#define MAX_FD_NUMBER 65535
#define MAX_EVENT_NUMBER 1024
#define TIME_INTERVAL 5

static int pipe_fd[2]; // 0是读的文件描述符，1是写的文件描述符
static SortedTimerList timer_list;
static int epoll_fd = 0;

void SetNonBlocking(int fd)
{
    int flag = fcntl(fd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flag);
}

// 向epoll中添加文件描述符
void AddFd(int epoll_fd, int fd, bool one_shot)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
    // if (one_shot)
    // {
    //     ev.events |= EPOLLONESHOT; 
    // }
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    SetNonBlocking(fd);
}

// 通知主程序已经接收到信号，将收到的信号写入管道
void InfoReceiveSig(int sig)
{
    // 通过保存和恢复 errno 值，确保不会干扰到主程序中的错误处理
    int now_errno = errno;
    send(pipe_fd[1], (void *)&sig, 1, 0);
    errno = now_errno;
}

// 添加信号捕捉
void AddSig(int sig)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = InfoReceiveSig;
    // 设置信号处理期间的系统调用行为，系统调用被中断后会自动恢复
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    if (sigaction(sig, &sa, NULL) == -1)
    {
        throw std::exception();
    }
}

// 定时器定时处理函数，调用 SortedTimerList 的 CleanTimeedOutTimer() 函数
void TimerHandler()
{
    timer_list.CleanTimeoutTimer();
    // 重新设置定时，因为一次 alarm() 只会引起一次 SIGALARM 信号
    alarm(TIME_INTERVAL);
}

// 将套接字从epoll实例中删除，关闭套接字
void RemoveFd( ClientData* user_data )
{
    printf("RemoveFd function\n");
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, user_data->m_socket_fd, nullptr);
    assert(user_data);
    if (user_data->m_socket_fd != -1)
    {
        close(user_data->m_socket_fd);
        printf("close fd %d\n", user_data->m_socket_fd);
        user_data->m_socket_fd = -1;
    }
}

int main(int argc, char *argv[])
{
    if (argc <= 1)
    {
        printf("usage: %s port number\n", basename(argv[0]));
        exit(-1);
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argv[1]));
    server_address.sin_addr.s_addr = INADDR_ANY;
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listen_fd >= 0);

    int reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    if (bind(listen_fd, (struct sockaddr *)(&server_address), sizeof(server_address)) == -1)
    {
        throw std::exception();
    }
    if (listen(listen_fd, 5) == -1)
    {
        throw std::exception();
    }

    epoll_event events[MAX_EVENT_NUMBER];
    epoll_fd = epoll_create(1);
    assert(epoll_fd != -1);
    AddFd(epoll_fd, listen_fd, false);

    // 创建管道
    if (socketpair(PF_UNIX, SOCK_STREAM, 0, pipe_fd) == -1)
    {
        throw std::exception();
    }
    SetNonBlocking(pipe_fd[1]);
    AddFd(epoll_fd, pipe_fd[0], true);

    // 设置信号处理函数
    AddSig(SIGALRM); // 定时器信号
    AddSig(SIGTERM); // 请求进程终止的信号
    bool stop_server = false;

    ClientData *user = new ClientData[MAX_FD_NUMBER];
    bool timeout = false;
    alarm(TIME_INTERVAL); //定时，5s 后产生 SIG_ALRM 信号

    while (!stop_server)
    {
        int number = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR)
        {
            perror("epoll_wait");
            exit(-1);
        }

        for (int i = 0; i < number; ++i)
        {
            int socket_fd = events[i].data.fd;
            if (socket_fd == listen_fd)
            {
                struct sockaddr_in client_address;
                socklen_t client_address_len = sizeof(client_address);
                int client_fd = accept(listen_fd, (struct sockaddr *)(&client_address), &client_address_len);
                AddFd(epoll_fd, client_fd, true);
                memcpy(&user[client_fd].m_address, &client_address, client_address_len);
                user[client_fd].m_socket_fd = client_fd;

                // 创建定时器，设置回调函数与超时时间，然后绑定定时器与用户数据，最后将定时器添加到链表timer_lst中
                time_t timeout_time = time(nullptr) + 3 * TIME_INTERVAL;
                Timer *timer = new Timer(timeout_time, RemoveFd, &user[client_fd]);
                user[client_fd].m_timer = timer;
                timer_list.AddTimer(timer);
            }
            // 处理信号
            else if (socket_fd == pipe_fd[0] && (events[i].events & EPOLLIN))
            {
                int sig;
                char signals[1024];
                int res = recv(socket_fd, signals, sizeof(signals), 0);
                if (res < 0)
                {
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                    {
                        AddFd(epoll_fd, socket_fd, true);
                    }
                    continue;
                }
                else if (res == 0)
                {
                    AddFd(epoll_fd, socket_fd, true);
                    continue;
                }
                else
                {
                    for (int j = 0; j < res; ++j)
                    {
                        switch(signals[j])
                        {
                            case SIGALRM:
                            {
                                // 用timeout变量标记有定时任务需要处理，但不立即处理定时任务
                                // 因为定时任务的优先级不是很高，我们优先处理其他更重要的任务
                                timeout = true;
                                break;
                            }
                            case SIGTERM:
                            {
                                stop_server = true;
                            }
                        }
                    }
                    AddFd(epoll_fd, socket_fd, true);
                }
            }
            else if (events[i].events & EPOLLIN)
            {
                memset(user[socket_fd].m_read_buf, 0, READ_BUFFER_SIZE);
                int len = recv(socket_fd, user[socket_fd].m_read_buf, READ_BUFFER_SIZE, 0);
                printf("get %d bytes info : %s from fd %d\n", len, user[socket_fd].m_read_buf, socket_fd);
                Timer *timer = user[socket_fd].m_timer;
                if (len < 0)
                {
                    // 发生错误，关闭连接，并将定时器从链表中删除
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                    {
                        RemoveFd(user + socket_fd);
                        if (timer)
                        {
                            timer_list.DeleteTimer(timer);
                        }
                    }
                    AddFd(epoll_fd, socket_fd, true);
                }
                else if (len == 0)
                {
                    // 对方关闭连接的话，我们也关闭连接并从链表中删除定时器
                    RemoveFd(user + socket_fd);
                    if (timer)
                    {
                        timer_list.DeleteTimer(timer);
                    }
                }
                else
                {
                    // 客户发来信息，调整该连接对应的定时器，以延迟该连接被关闭的时间
                    if (timer)
                    {
                        timer->SetTimeoutTime(time(nullptr) + 3 * TIME_INTERVAL);
                        printf("client %d send info, adjust timeout time to %d\n", socket_fd, 3 * TIME_INTERVAL);
                        timer_list.AdjustTimer(timer);
                    }
                    AddFd(epoll_fd, socket_fd, true);
                }
            }
        }
        // 最后处理定时事件,清理长期未发送数据的用户。因为I/O事件有更高的优先级
        if (timeout)
        {
            TimerHandler();
            timeout = false;
        }
    }
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    close(listen_fd);
    close(epoll_fd);
    delete [] user;
    return 0;
}