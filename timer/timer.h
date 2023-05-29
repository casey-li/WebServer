#ifndef TIMER_H
#define TIMER_H
#include <arpa/inet.h>
#include <ctime>
#include <string.h>
#include <cstdio>
#include <unistd.h>

#define READ_BUFFER_SIZE 64
class Timer;

// 用户数据
class ClientData
{
public:
    ClientData();
    ~ClientData();
    ClientData(sockaddr_in &addr, int fd);
    sockaddr_in m_address;              // 客户端 socket 地址
    int m_socket_fd;                    // 文件描述符
    char m_read_buf[READ_BUFFER_SIZE];  // 读缓冲区
    Timer *m_timer;                     // 定时器
};

// 定时器类
class Timer
{
public:
    Timer();
    ~Timer();
    Timer(time_t time, void(handler)(ClientData*), ClientData * user_data);
    void SetTimeoutTime(time_t new_time);   // 重设定时器超时时间
    time_t m_time;                  // 任务超时时间（绝对时间）
    void (*DealTask)(ClientData*);  // 任务回调函数，处理的数据由定时器的执行者传递给回调函数
    ClientData *m_user_data;        // 用户数据
    Timer *m_prev;                  // 指向前一个定时器
    Timer *m_next;                  // 指向后一个定时器
};

// 定时器链表，双向升序链表
class SortedTimerList
{
public:
    SortedTimerList();
    ~SortedTimerList();
    void AddTimer(Timer *timer);    // 将目标定时器添加到链表中
    void AdjustTimer(Timer *timer); // 任务定时发生改变（仅考虑再次有请求过来，延长定时时间），调整定时器的位置
    void DeleteTimer(Timer *timer); // 将定时器从链表中删除
    void CleanTimeoutTimer();     // SIGALARM 信号被触发后，清理所有超时的定时器

private:
    void AddTimer(Timer *timer, Timer *loction); // 重载函数，将 timer 插入到 location 之后的链表中
    void AdjustPointer();   // 调整指针朝向，头指针指向尾指针，尾指针指向头指针
    Timer *m_head;          // 有序双向链表头结点
    Timer *m_tail;          // 有序双向链表尾结点
};


#endif