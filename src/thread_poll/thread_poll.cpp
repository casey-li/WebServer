#include "thread_poll.h"

explicit ThreadPoll::ThreadPoll(size_t thread_num = 8) : m_poll(std::make_shared<Poll>())
{
    assert(thread_num > 0);
    m_poll->m_is_stop = false;
    // 创建线程并进行线程分离
    for (size_t i = 0; i < thread_num; ++i)
    {
        // c++ 的 std::thread 可以灵活的使用不同签名的工作函数
        // c 的 pthread.h 只接受 void *(*)(void *) 签名的函数 （要将工作函数设为全局函数或者静态成员函数）
        // 传递一个函数指针，并将 this 指针做为参数传递给它，以绑定成员函数和实例对象
        std::thread(&ThreadPoll::Work, this).detach();
    }
}

ThreadPoll::~ThreadPoll()
{
    if (m_poll)
    {
        {
            // std::lock_guard 是一个简单的管理互斥锁的类，仅支持在作用域内创建时加锁，析构时解锁
            // 不支持手动加锁解锁
            std::lock_guard<std::mutex> locker(m_poll->m_mutex);
            m_poll->m_is_stop = true;
        }
        m_poll->m_cond.notify_all();
    }
}

void ThreadPoll::Work()
{
    // 创建互斥锁封装类 std::unique_locker<std::mutex> 用于管理互斥锁，构造函数会自动加锁，析构函数自动解锁
    // 此外，它还支持手动加锁解锁
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

template<typename T>
void ThreadPoll::AddTask(T&& task)
{
    {
        std::lock_guard<std::mutex> locker(m_poll->m_mutex);
        // 使用 std::forward<T> 实现完美转发，确保参数的值类别保持不变。
        // 若 task 是左值引用，转发后仍为左值引用；若 task 为右值引用，转发后仍为右值引用
        // 这样可以最大限度地减少不必要的拷贝或移动操作
        m_poll->m_tasks.emplace(std::forward<T>(task));
    }
    m_poll->m_cond.notify_one();
}