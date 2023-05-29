// 线程池类，定义为模板类可以方便地进行代码复用，处理各种类型的任务
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <list>
#include <semaphore>
#include <cstdio>
#include "locker.hpp"

template <typename T>
class ThreadPool
{
public:
    // thread_number是线程池中线程的最大数量，max_requests是请求队列中最多允许的、等待处理的请求的数量
    ThreadPool(int thread_num = 8, int max_requests = 100000);

    ~ThreadPool();

    bool append(T *request);

private:
    // 工作线程运行的函数，它不断从工作队列中取出任务并执行，必须设为静态函数，这样等价于全局函数
    // 若为非静态函数则需要通过类的对象调用，对象生命周期结束后线程工作函数也就不能用了
    // 更重要的是非静态成员函数都会在参数列表中隐含一个this指针做为参数，不满足线程函数签名的要求 (void *(*)(void *))
    static void *work(void *arg);

    void run();

private:
    // 线程的数量
    int m_thread_number;

    // 请求队列中最多允许的，等待处理的请求的数目
    int m_max_requests;

    // 描述线程池的数组，大小为 m_thread_number
    pthread_t *m_pthreads;

    // 请求队列
    std::list<T *> m_work_queue;

    // 保护请求队列的互斥锁
    Locker m_work_queue_locker;

    // 信号量，表明是否有任务需要处理
    Sem m_queue_stat;

    // 是否结束线程
    bool m_stop;
};

template <typename T>
ThreadPool<T>::ThreadPool(int thread_number, int max_requests) : m_thread_number(thread_number),
                                                                 m_max_requests(max_requests), m_pthreads(NULL), m_stop(false)
{

    if (thread_number <= 0 || max_requests <= 0)
    {
        throw std::exception();
    }
    m_pthreads = new pthread_t[m_thread_number];
    if (!m_pthreads)
    {
        throw std::exception();
    }
    // 创建 thread_number 个线程，并设置线程分离
    for (int i = 0; i < thread_number; ++i)
    {
        printf("creater the %dth thread...\n", i);
        if (pthread_create(m_pthreads + i, NULL, work, this) != 0)
        {
            delete[] m_pthreads;
            throw std::exception();
        }
        if (pthread_detach(m_pthreads[i]) != 0)
        {
            delete[] m_pthreads;
            throw std::exception();
        }
    }
}

template <typename T>
ThreadPool<T>::~ThreadPool()
{
    delete[] m_pthreads;
    m_stop = true;
}

template <typename T>
bool ThreadPool<T>::append(T *request)
{
    // 操作工作队列时一定要加锁，因为它被所有线程共享
    m_work_queue_locker.Lock();

    // 等于？？？

    if (m_work_queue.size() > m_max_requests)
    {
        m_work_queue_locker.UnLock();
        return false;
    }
    m_work_queue.emplace_back(request);
    m_work_queue_locker.UnLock();
    m_queue_stat.Post(); // 发送信号，唤醒等待进程，让其去工作
    return true;
}

// 静态成员函数不能操控非静态成员函数，而我们又需要调用其中的函数，因此直接将对象的地址传给arg
template <typename T>
void *ThreadPool<T>::work(void *arg)
{
    ThreadPool *pool = static_cast<ThreadPool *>(arg);
    pool->run();
    return pool;
}

template <typename T>
void ThreadPool<T>::run()
{
    while (!m_stop)
    {
        m_queue_stat.Wait();        // 减少信号量的值
        m_work_queue_locker.Lock(); // 加锁
        if (m_work_queue.empty())
        {
            m_work_queue_locker.UnLock();

            // 信号量不改回来？？？

            continue;
        }
        T *request = m_work_queue.front();
        m_work_queue.pop_front();
        m_work_queue_locker.UnLock();
        if (!request)
        {
            continue;
        }
        request->Process(); // 调用工作T的处理函数
    }
}

#endif