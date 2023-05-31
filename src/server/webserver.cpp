#include "webserver.h"

/*
TODO:
1、HttpConnection的静态成员初始化
2、Sql连接池的初始化
3、设置日志相关信息
*/
WebServer::WebServer(int port, int trigger_mode, int connect_poll_num, int thread_num) :
                    m_port(port), m_is_close(false), m_listen_event(0), m_connection_event(0),
                    m_resource_dir("/home/casey/niuke/webserver/resources/"), 
                    m_thread_poll(std::make_unique<ThreadPoll>(thread_num)), m_epoller(std::make_unique<Epoller>())
{
    // HttpConnection::m_connection_number = 0;
    // HttpConnection::m_resource_dir = m_resource_dir;
    // SqlConnection::
    InitEventMode(trigger_mode);
    if (!InitSocket()) 
    {
        m_is_close = true;
    }

    // 根据是否需要打印日志设置相关信息 
}

// todo : 关闭sql池
WebServer::~WebServer()
{
    close(m_listen_fd);
    m_is_close = true;
    // 关闭sql池
}


// TODO 定时器模块，设置 epoll_wait() 的阻塞时间
void WebServer::Start()
{
    int timeout = -1; // 默认epoll_wait 阻塞
    if (!m_is_close)
    {
        // 打印日志
    }
    while (!m_is_close)
    {
        // 根据定时器设置timeout
        int event_number = m_epoller->Wait(timeout);
        for (int i = 0; i < event_number; ++i)
        {
            int fd = m_epoller->GetEventFd(i);
            uint32_t event = m_epoller->GetEvents(i);
            if (fd == m_listen_event)
            {
                DealListen();
            }
            else if (event & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                assert(m_users.count(fd) > 0);
                CloseConnection(m_users[fd]);
            }
            else if (event & EPOLLIN)
            {
                assert(m_users.count(fd) > 0);
                DealRead(m_users[fd]);
            }
            else if (event & EPOLLOUT)
            {
                assert(m_users.count(fd) > 0);
                DealWrite(m_users[fd]);
            }
            else
            {
                // 打印日志
            }
        }
    }
}

// TODO: 设置HtppConnection 的 ET 模式
void WebServer::InitEventMode(int trigger_mode)
{
    // EPOLLRDHUP 表示对端（对于 TCP 连接）关闭了写通道或关闭了连接
    // EPOLLHUP 表示发生了挂起事件，通常与文件描述符相关的异常情况有关，如管道破裂、连接被重置等
    // EPOLLERR 表示发生了错误事件，通常表示出现了与文件描述符相关的错误，如连接错误、I/O 错误等
    // EPOLLONESHOT 表示注册的文件描述符只能触发一次事件，触发后需要重新设置才能继续触发。这可以用于实现一次性的事件处理
    m_listen_event = EPOLLRDHUP | EPOLLHUP | EPOLLERR;
    m_connection_event = EPOLLONESHOT | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
    switch (trigger_mode)
    {
        case 0:
            break;
        case 1:
            m_connection_event |= EPOLLET;
            break;
        case 2:
            m_listen_event |= EPOLLET;
            break;
        case 3:
        default:
            m_listen_event |= EPOLLET;
            m_connection_event |= EPOLLET;
            break;
    }
    // HttpConnection::
}

// TODO：设置优雅关闭，打印日志信息
bool WebServer::InitSocket()
{
    if (m_port > 65535 || m_port < 1024)
    {
        // 打印日志
        return false;
    }

    if ((m_listen_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        // 打印日志
        return false;
    }

    // 根据参数设置是否优雅关闭
    // setsockopt 设置 SO_LINGER 属性

    int reuse = 1;
    if ((setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) == -1)
    {
        // 打印日志
        close(m_listen_fd);
        return false;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);
    if (bind(m_listen_fd, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        // 打印日志
        close(m_listen_fd);
        return false;
    }

    if (listen(m_listen_fd, 5) == -1)
    {
        // 打印日志
        close(m_listen_fd);
        return false;
    }

    if (!m_epoller->AddFd(m_listen_fd, m_listen_event | EPOLLIN))
    {
        // 打印日志
        close(m_listen_fd);
        return false;
    }

    if (!SetNonblock(m_listen_fd))
    {
        // 打印日志
    }

    // 打印日志
    return true;
}

void WebServer::DealListen()
{
    struct sockaddr_in client_address;
    socklen_t len = sizeof(client_address);
    // 若监听事件设为了 ET 模式，需要一次性处理所有连接的请求（do while）
    do
    {
        int fd = accept(m_listen_fd, (struct sockaddr *)&client_address, &len);
        if (fd < 0)
        {
            return;
        }
        else if (HttpConnection::m_http_connection_numner >= MAX_FD)
        {
            SendError(fd, "Server is busy now!");
            // 打印日志
            return;
        }
        AddClient(fd, client_address);
    } while (m_listen_event & EPOLLET);
}

// TODO 增加定时器功能，处理长久未发送信息的客户端
void WebServer::AddClient(int fd, const sockaddr_in &address)
{
    assert(fd > 0);
    m_users[fd].Initialization(fd, address);
    // 加入到定时器链表中
    m_epoller->AddFd(fd, m_listen_event | EPOLLIN);
    SetNonblock(fd);
    // 打印日志
}

void WebServer::CloseConnection(HttpConnection &client)
{
    assert(client.GetFd() > 0);
    // 打印日志
    m_epoller->DeleteFd(client.GetFd());
    client.Close();
}

// TODO 定时器修改客户端的最近访问事件
void WebServer::DealRead(HttpConnection &client)
{
    assert(client.GetFd() > 0);
    // 更新当前客户端的最近请求事件，在定时器中修改内容

    // 使用 bind 将成员函数修改为 void(*)() 的可调用对象（绑定成员函数，必须传递 this），右值 
    m_thread_poll->AddTask(std::bind(&WebServer::ReadTask, this, std::ref(client)));
}

void WebServer::ReadTask(HttpConnection &client)
{
    assert(client.GetFd() > 0);
    int res = -1, error_num = 0;
    res = client.Read(error_num);
    if (res <= 0 && error_num != EAGAIN)
    {
        CloseConnection(client);
        return;
    }
    Process(client);
}

// TODO 定时器修改客户端的最近访问事件
void WebServer::DealWrite(HttpConnection &client)
{
    assert(client.GetFd() > 0);
    // 更新当前客户端的最近请求事件，在定时器中修改内容
    m_thread_poll->AddTask(std::bind(&WebServer::WriteTask, this, std::ref(client)));
}

void WebServer::WriteTask(HttpConnection &client)
{
    assert(client.GetFd() > 0);
    int res = -1, error_num = 0;
    // 由客户端保证在不出错的前提下一次性写完所有数据
    res = client.Write(error_num);
    if (client.GetToWriteBytes() == 0)
    {
        // 写任务处理完毕
        if (client.IsKeepAlive())
        {
            Process(client);
            return;
        }
    }
    if (res < 0 && error_num == EAGAIN)
    {
        // 继续传输
        m_epoller->ModifyFd(client.GetFd(), m_connection_event | EPOLLOUT);
        return;
    }
    CloseConnection(client);
}

void WebServer::Process(HttpConnection &client)
{
    if (client.Process())
    {
        m_epoller->ModifyFd(client.GetFd(), m_connection_event | EPOLLOUT);
    }
    else
    {
        m_epoller->ModifyFd(client.GetFd(), m_connection_event | EPOLLIN);
    }
}

void WebServer::SendError(int fd, const std::string &erro_info)
{
    assert(fd > 0);
    if (send(fd, erro_info.c_str(), erro_info.size(), 0) == -1)
    {
        // 打印日志
    }
    close(fd);
}

bool WebServer::SetNonblock(int fd)
{
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) == 0;
}
