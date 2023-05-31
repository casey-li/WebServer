#include "thread_poll.h"

ThreadPoll::ThreadPoll(size_t thread_num) : m_poll(std::make_shared<Poll>())
{
    assert(thread_num > 0);
    m_poll->m_is_stop = false;
    // 创建线程并进行线程分离
    for (size_t i = 0; i < thread_num; ++i)
    {
        // c++ 的 std::thread 可以灵活的使用不同签名的工作函数
        // c 的 pthread.h 只接受 void *(*)(void *) 签名的函数 （要将工作函数设为全局函数或者静态成员函数）
        // 传递一个函数指针，并将 this 指针做为参数传递给它（成员函数有默认的this指针做为参数）
        std::thread(&ThreadPoll::Work, this).detach();
    }
}

ThreadPoll::~ThreadPoll()
{
    if (m_poll)
    {
        {
            std::lock_guard<std::mutex> locker(m_poll->m_mutex);
            m_poll->m_is_stop = true;
        }
        m_poll->m_cond.notify_all();
    }
}

void ThreadPoll::Work()
{
    std::unique_lock<std::mutex> locker(m_poll->m_mutex); 
    while (true)
    {
        if (!m_poll->m_tasks.empty())
        {
            auto task = std::move(m_poll->m_tasks.front());
            m_poll->m_tasks.pop();
            locker.unlock(); // 解锁，让其他线程也可以去取任务工作
            task();          // 因为工作队列中保存的就是可调用对象，可以直接执行任务
            locker.lock();
        }
        else if (m_poll->m_is_stop) break;
        else m_poll->m_cond.wait(locker); // 当前无任务，等待唤醒
    }
}

