#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <unordered_map>

#include "../thread_poll/thread_poll.h"
#include "../epoller/epoller.h"
#include "../http/http_connection.h"

class WebServer
{
public:
    WebServer(int port, int trigger_mode, int connect_poll_num, int thread_num);

    ~WebServer();

    // 启动服务器，调用 epoll_wait() 监听事件
    void Start();

private: // 主线程调用的函数
    // 根据传入的参数设置客户和监听的默认触发模式 (是否ET，并设置其它事件)
    // 0 表示不设置ET；1 表示设置来连接的ET；2 表示设置监听 ET；3以及其它情况表示连接和监听都设置 ET
    void InitEventMode(int trigger_mode); 

    // 初始化socket，完成绑定，监听操作
    bool InitSocket();
    
    // 当检测到有新的连接时，接受连接并进行相应的处理
    void DealListen();

    // 初始化一个 HttpConnection 对象，并加入到 epoll 对象中监听
    void AddClient(int fd, const sockaddr_in &address);

    // 关闭一个客户端的连接，从 epoll 对象中删除并调用客户端的 Close()
    void CloseConnection(HttpConnection &client);

    // 更新当前客户的最近访问时间，并将读任务 (ReadTask) 交给工作队列，让子线程去处理
    void DealRead(HttpConnection &client);

    // 更新当前客户的最近访问时间，并将写任务 (WriteTask) 交给工作队列，让子线程去处理
    void DealWrite(HttpConnection &client);

    // 发送错误信息
    void SendError(int fd, const std::string &erro_info);

    // 设置文件描述符的属性为非阻塞
    bool SetNonblock(int fd);

private: // 子线程调用的函数
    // 调用 HttpConnection 的 Read()，读取成功的话对读取的请求进行处理
    void ReadTask(HttpConnection &client);

    // 调用 HttpConnection 的 Write
    void WriteTask(HttpConnection &client);

    // 调用 HttpConnection 的 Process，解析请求并生成响应。若成功则修改监听事件为写事件，否则继续监听读事件
    void Process(HttpConnection &client);

private:

    static const int MAX_FD_ = 64435;
    int port_;
    bool is_close_;
    int listen_fd_;
    std::string resource_dir_;

    uint32_t listen_event_;
    uint32_t connection_event_;

    std::unique_ptr<ThreadPoll> thread_poll_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConnection> users_;
};


#endif