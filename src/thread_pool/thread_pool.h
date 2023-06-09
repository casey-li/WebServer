#ifndef THREADPOLL_H
#define THREADPOLL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <cassert>

#include <iostream>

class ThreadPool
{
public:
    ThreadPool() = default;

    explicit ThreadPool(size_t thread_num = 8);

    ~ThreadPool();

    void Work();

    template<typename T>
    void AddTask(T&& task);

private:
    // 结构体，池子
    struct Pool
    {
        std::mutex mtx;                         // 互斥锁
        std::condition_variable cond;             // 条件变量
        bool is_stop;                             // 线程池是否停止
        std::queue<std::function<void()>> tasks;  // 任务队列，存储无参且返回值为 void 的可调用对象 （待执行的任务）
    };
    
    std::shared_ptr<Pool> pool_; // 池子
};

template<typename T>
void ThreadPool::AddTask(T&& task)
{
    {
        std::lock_guard<std::mutex> locker(pool_->mtx);
        // 使用 std::forward<T> 实现完美转发，确保参数的值类别保持不变。
        pool_->tasks.emplace(std::forward<T>(task));
    }
    pool_->cond.notify_one();
}

#endif