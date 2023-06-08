#include "webserver.h"

WebServer::WebServer(
        int port, int timeout, int trigger_mode, bool open_linger_, int connect_poll_num, 
        int thread_num, bool open_log, int log_level, int block_queue_size) :
        port_(port), listen_fd_(-1), timeout_MS_(timeout), is_close_(false), 
        open_linger_(open_linger_), resource_dir_(""), listen_event_(0), 
        connection_event_(0), thread_poll_(std::make_unique<ThreadPoll>(thread_num)),
        epoller_(std::make_unique<Epoller>()), timer_(std::make_unique<HeapTimer>())
{
    std::string tmp = getcwd(nullptr, 256);
    size_t end = tmp.find("/src");
    resource_dir_ = end != std::string::npos ? tmp.substr(0, end) : tmp;
    assert(resource_dir_.size());
    resource_dir_ += "/resources/";
    HttpConnection::http_connection_numner_ = 0;
    HttpConnection::resource_dir_ = resource_dir_;
    if (open_log)
    {
        Log::GetInstance()->Initialization(log_level, "./log", ".log", block_queue_size);
    }
    MysqlConnectionPool::GetInstance();
    InitEventMode(trigger_mode);
    if (!InitSocket()) 
    {
        is_close_ = true;
    }
    // 根据是否需要打印日志设置相关信息
    if (open_log)
    {
        // Log::GetInstance()->Initialization(log_level, "./log", ".log", block_queue_size);
        if (is_close_)
        {
            LOG_ERROR("========== Server initialization error! ==========");
        }
        else
        {
            LOG_INFO("========== Server initialization ==========");
            LOG_INFO("Port: %d, OpenLinger: %s", port_, open_linger_? "true" : "false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s", (listen_event_ & EPOLLET ? "ET": "LT"), (connection_event_ & EPOLLET ? "ET": "LT"));
            LOG_INFO("Log level: %d", log_level);
            LOG_INFO("ResourceDir: %s", HttpConnection::resource_dir_.c_str());
            LOG_INFO("MySql connect database : %s", MysqlConnectionPool::GetInstance()->GetDatabaseName().c_str());
        }
    }
}

WebServer::~WebServer()
{
    close(listen_fd_);
    is_close_ = true;
    MysqlConnectionPool::GetInstance()->CloseMysqlConnectionPool();
}

void WebServer::Start()
{
    int timeout = -1; // 默认epoll_wait 阻塞
    if (!is_close_)
    {
        LOG_INFO("========== Start Server ==========");
    }
    while (!is_close_)
    {
        // 断开超时的连接，设置 epoll_wait() 的阻塞时间为最早的未超时节点到超时需要的时间
        if (timeout_MS_ > 0) 
        {
            timeout = timer_->GetNextTimeout();
        }
        int event_number = epoller_->Wait(timeout);
        for (int i = 0; i < event_number; ++i)
        {
            int fd = epoller_->GetEventFd(i);
            uint32_t event = epoller_->GetEvents(i);
            if (fd == listen_fd_)
            {
                DealListen();
            }
            else if (event & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                assert(users_.count(fd) > 0);
                CloseConnection(users_[fd]);
            }
            else if (event & EPOLLIN)
            {
                assert(users_.count(fd) > 0);
                DealRead(users_[fd]);
            }
            else if (event & EPOLLOUT)
            {
                assert(users_.count(fd) > 0);
                DealWrite(users_[fd]);
            }
            else
            {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

void WebServer::InitEventMode(int trigger_mode)
{
    // EPOLLRDHUP 表示对端（对于 TCP 连接）关闭了写通道或关闭了连接
    // EPOLLHUP 表示发生了挂起事件，通常与文件描述符相关的异常情况有关，如管道破裂、连接被重置等
    // EPOLLERR 表示发生了错误事件，通常表示出现了与文件描述符相关的错误，如连接错误、I/O 错误等
    // EPOLLONESHOT 表示注册的文件描述符只能触发一次事件，触发后需要重新设置才能继续触发。这可以用于实现一次性的事件处理
    listen_event_ = EPOLLRDHUP | EPOLLHUP | EPOLLERR;
    connection_event_ = EPOLLONESHOT | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
    switch (trigger_mode)
    {
        case 0:
            break;
        case 1:
            connection_event_ |= EPOLLET;
            break;
        case 2:
            listen_event_ |= EPOLLET;
            break;
        case 3:
        default:
            listen_event_ |= EPOLLET;
            connection_event_ |= EPOLLET;
            break;
    }
    HttpConnection::is_ET_mode_ = connection_event_ & EPOLLET;
}

bool WebServer::InitSocket()
{
    if (port_ > 65535 || port_ < 1024)
    {
        LOG_ERROR("Port: %d error!",  port_);
        return false;
    }

    if ((listen_fd_ = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        LOG_ERROR("Create socket error!");
        return false;
    }

    // 根据参数设置是否优雅关闭，设置超时时间为 1s
    struct linger opt_linger = {0};
    if (open_linger_)
    {
        opt_linger.l_onoff = 1;
        opt_linger.l_linger = 1;
    }
    if (setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, &opt_linger, sizeof(opt_linger)) == -1)
    {
        LOG_ERROR("Init linger error!");
        close(listen_fd_);
        return false;
    }

    // 设置端口复用
    int reuse = 1;
    if ((setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) == -1)
    {
        LOG_ERROR("set socket setsockopt error !");
        close(listen_fd_);
        return false;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port_);
    if (bind(listen_fd_, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        LOG_ERROR("Bind Port: %d error!", port_);
        close(listen_fd_);
        return false;
    }

    if (listen(listen_fd_, 5) == -1)
    {
        LOG_ERROR("Listen port: %d error!", port_);
        close(listen_fd_);
        return false;
    }

    if (!epoller_->AddFd(listen_fd_, listen_event_ | EPOLLIN))
    {
        LOG_ERROR("Add listen error!");
        close(listen_fd_);
        return false;
    }

    if (!SetNonblock(listen_fd_))
    {
        LOG_ERROR("Set File Nonblock failed!");
    }
    LOG_INFO("Init Socket Success! listen fd : %d", listen_fd_);
    return true;
}

void WebServer::DealListen()
{
    struct sockaddr_in client_address;
    socklen_t len = sizeof(client_address);
    // 若监听事件设为了 ET 模式，需要一次性处理所有连接的请求（do while）
    do
    {
        int fd = accept(listen_fd_, (struct sockaddr *)&client_address, &len);
        if (fd < 0)
        {
            return;
        }
        else if (HttpConnection::http_connection_numner_ >= MAX_FD_)
        {
            SendError(fd, "Server is busy now!");
            LOG_WARN("Clients is full!");
            return;
        }
        AddClient(fd, client_address);
    } while (listen_event_ & EPOLLET);
}

void WebServer::AddClient(int fd, const sockaddr_in &address)
{
    assert(fd > 0);
    users_[fd].Initialization(fd, address);
    if (timeout_MS_ > 0) // 加入到定时器链表中
    {
        timer_->Add(fd, timeout_MS_, std::bind(&WebServer::CloseConnection, this, std::ref(users_[fd])));
    }
    epoller_->AddFd(fd, listen_event_ | EPOLLIN);
    SetNonblock(fd);
    LOG_INFO("Client[%d] in!", users_[fd].GetFd());
}

void WebServer::CloseConnection(HttpConnection &client)
{
    assert(client.GetFd() > 0);
    LOG_INFO("Client[%d] quit!", client.GetFd());
    epoller_->DeleteFd(client.GetFd());
    client.Close();
}

void WebServer::UpdateClientTimeout(HttpConnection &client)
{
    assert(client.GetFd() > 0);
    if (timeout_MS_ > 0)
    {
        timer_->AdjustTime(client.GetFd(), timeout_MS_);
    }
}

void WebServer::DealRead(HttpConnection &client)
{
    assert(client.GetFd() > 0);
    UpdateClientTimeout(client);
    // 使用 bind 将成员函数修改为 void(*)() 的可调用对象（绑定成员函数，必须传递 this），右值 
    thread_poll_->AddTask(std::bind(&WebServer::ReadTask, this, std::ref(client)));
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

void WebServer::DealWrite(HttpConnection &client)
{
    assert(client.GetFd() > 0);
    UpdateClientTimeout(client);
    thread_poll_->AddTask(std::bind(&WebServer::WriteTask, this, std::ref(client)));
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
        epoller_->ModifyFd(client.GetFd(), connection_event_ | EPOLLOUT);
        return;
    }
    CloseConnection(client);
}

void WebServer::Process(HttpConnection &client)
{
    if (client.Process())
    {
        epoller_->ModifyFd(client.GetFd(), connection_event_ | EPOLLOUT);
    }
    else
    {
        epoller_->ModifyFd(client.GetFd(), connection_event_ | EPOLLIN);
    }
}

void WebServer::SendError(int fd, const std::string &erro_info)
{
    assert(fd > 0);
    if (send(fd, erro_info.c_str(), erro_info.size(), 0) == -1)
    {
        LOG_WARN("send error to client[%d] !", fd);
    }
    close(fd);
}

bool WebServer::SetNonblock(int fd)
{
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) == 0;
}
