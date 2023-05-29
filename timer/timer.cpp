#include "timer.h"

ClientData::ClientData() : m_socket_fd(-1), m_timer(nullptr)
{
    memset(&m_address, 0, sizeof(m_address));
    memset(m_read_buf, 0, sizeof(m_read_buf));
}

ClientData::~ClientData()
{
    delete m_timer;
    memset(&m_address, 0, sizeof(m_address));
    if (m_socket_fd != -1)
    {
        close(m_socket_fd);
        m_socket_fd = -1;
    }
    memset(m_read_buf, 0, sizeof(m_read_buf));
}

Timer::Timer() : m_time(0), m_user_data(nullptr), m_prev(nullptr), m_next(nullptr)
{
}

Timer::Timer(time_t time, void(handler)(ClientData*), ClientData * user_data)
            : m_time(time), DealTask(handler), m_user_data(user_data), m_prev(nullptr), m_next(nullptr)
{

}

// 重设定时器超时时间
void Timer::SetTimeoutTime(time_t new_time)
{
    this->m_time = new_time;
}

Timer::~Timer()
{
    m_time = 0;
    m_prev = m_next = nullptr;
}

SortedTimerList::SortedTimerList() : m_head(nullptr), m_tail(nullptr)
{
}

SortedTimerList::~SortedTimerList()
{
    Timer *tmp = m_head;
    while (m_head)
    {
        tmp = m_head->m_next;
        m_head->m_prev = m_head->m_next = nullptr;
        delete m_head;
        m_head = tmp;
    }
}

// 将目标定时器添加到链表中
void SortedTimerList::AddTimer(Timer *timer)
{
    if (!timer)
    {
        return;
    }
    if (!m_head)
    {
        m_head = m_tail = timer;
        AdjustPointer();
        return;
    }
    // 定时时间比头结点还小，插在头之前
    if (timer->m_time < m_head->m_time)
    {
        timer->m_next = m_head;
        m_head->m_prev = timer;
        m_head = timer;
        AdjustPointer();
    }
    AddTimer(timer, m_head);
}

// 从 location 开始向后找第一个时间大于 timer 的节点并将 timer 插到该节点之前
void SortedTimerList::AddTimer(Timer *timer, Timer *location)
{
    while (location != m_head)
    {
        if (location->m_time > timer->m_time)
        {
            break;
        }
        location = location->m_next;
    }
    // 没有哪个节点的剩余时间大于目标时间，插入到最后
    if (location == m_head)
    {
        m_tail->m_next = timer;
        timer->m_prev = m_tail;
        m_tail = timer;
        AdjustPointer(); 
    }
    else
    {
        timer->m_prev = location->m_prev;
        location->m_prev->m_next = timer;
        timer->m_next = location;
        location->m_prev = timer;
    }
}

// 任务定时发生改变，将目标定时器重新插入到合适位置
void SortedTimerList::AdjustTimer(Timer *timer)
{
    if (!timer)
    {
        return;
    }
    Timer *tmp = timer->m_next;
    // 如果当前定时器本来就是尾节点或者时间增大了以后下一个节点的时间仍大于当前节点则无需调整
    if (tmp == m_head || tmp->m_time > timer->m_time)
    {
        return;
    }
    // 先让该节点的前后节点相连，再重新插入该节点
    tmp->m_prev = timer->m_prev;
    timer->m_prev->m_next = tmp;
    // 若 timer 为头结点，注意修改头结点
    if (timer == m_head)
    {
        m_head = tmp;
        AdjustPointer();
    }
    AddTimer(timer, tmp);
}

// 将定时器从链表中删除
void SortedTimerList::DeleteTimer(Timer *timer)
{
    if (!timer)
    {
        return;
    }
    // 目标定时器是头结点或尾节点
    if (timer == m_head || timer == m_tail)
    {
        if (m_head == m_tail) // 只有一个节点
        {
            m_head = m_tail = nullptr;
        }
        else if (timer == m_head)
        {
            m_head = timer->m_next;
            AdjustPointer();
        }
        else
        {
            m_tail = timer->m_prev;
            AdjustPointer();
        }
    }
    else
    {
        timer->m_next->m_prev = timer->m_prev;
        timer->m_prev->m_next = timer->m_next;
    }
    delete timer;
    return;
}

// SIGALARM 信号被触发一次就清理一次所有超时的定时器
void SortedTimerList::CleanTimeoutTimer()
{
    if (!m_head)
    {
        return;
    }
    printf("receiev SIGALARM!\n");
    time_t cur_time = time(NULL); // 获取系统当前时间
    Timer *cur = m_head;
    while (m_head != m_tail)
    {
        printf("client time : %ld, cur time: %ld \n", cur->m_time, cur_time);
        if (cur->m_time > cur_time)
        {
            break;
        }
        // 保存下一个节点，再执行回调函数，删除当前节点
        m_head = cur->m_next;
        // 调用定时器的回调函数，以执行定时任务
        cur->DealTask(cur->m_user_data);
        delete cur;
        cur = m_head;
    }
    printf("client time : %ld, cur time: %ld \n", cur->m_time, cur_time);
    if (m_head == m_tail && m_head->m_time <= cur_time)
    {
        printf("deleting...\n");
        // 调用定时器的回调函数，以执行定时任务
        m_head->DealTask(cur->m_user_data);
        delete m_head;
        m_head = m_tail = nullptr;
        return;
    }
    AdjustPointer();
}

// 头节点的上一个节点指向尾节点，尾结点的下一个节点指向头结点
void SortedTimerList::AdjustPointer()
{
    m_head->m_prev = m_tail;
    m_tail->m_next = m_head;
}