#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <cassert>

class Epoller
{
public:
    explicit Epoller(int max_events = 1024);

    ~Epoller();

    bool AddFd(int fd, uint32_t event);     // 往epoll对象中添加fd，event为监听的事件
    
    bool ModifyFd(int fd, uint32_t event);  // 修改epoll对象中监听的fd的事件
    
    bool DeleteFd(int fd);                  // 从epoll对象中移除监听的fd
    
    int Wait(int timeout_ms = -1);          // 调用epoll_wait() 获取哪些fd对应的事件发生了改变
    
    int GetEventFd(size_t i) const;         // 返回成功监听到的发生变化的客户端对应的fd
    
    uint32_t GetEvents(size_t i) const;     // 返回成功监听到的发生变化的客户端的监听事件

private:
    int epoll_fd_; // 创建的epoll对象的fd
    
    std::vector<struct epoll_event> events_; // 调用epoll_wait() 传递的数组
};

#endif