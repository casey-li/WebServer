#ifndef THREADPOLL_H
#define THREADPOLL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <cassert>

class ThreadPoll
{
public:
    ThreadPoll() = default;

    explicit ThreadPoll(size_t thread_num = 8);

    ~ThreadPoll();

    void Work();

    template<typename T>
    void AddTask(T&& task);

private:
    // 结构体，池子
    struct Poll
    {
        std::mutex m_mutex;                         // 互斥锁
        std::condition_variable m_cond;             // 条件变量
        bool m_is_stop;                             // 线程池是否停止
        std::queue<std::function<void()>> m_tasks;  // 任务队列，存储无参且返回值为 void 的可调用对象 （待执行的任务）
    };
    
    std::shared_ptr<Poll> m_poll; // 池子
};

// 将函数模板的声明和定义放在一起，不然编译报错，无法实例化模板函数
template<typename T>
void ThreadPoll::AddTask(T&& task)
{
    {
        std::lock_guard<std::mutex> locker(m_poll->m_mutex);
        // 使用 std::forward<T> 实现完美转发，确保参数的值类别保持不变。
        m_poll->m_tasks.emplace(std::forward<T>(task));
    }
    m_poll->m_cond.notify_one();
}

#endif