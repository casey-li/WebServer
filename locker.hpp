// 线程同步机制封装类，包括 互斥锁，条件变量，和信号
#ifndef LOCKER_H
#define LOCKER_H

#include <pthread.h>
#include <exception>
#include <semaphore.h>

// 互斥锁类
class Locker
{
public:
    Locker()
    {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw std::exception();
        }
    }

    ~Locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    bool Lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }

    bool UnLock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }

    pthread_mutex_t *get_my_locker()
    {
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;
};

// 条件变量类
class Cond
{
public:
    Cond()
    {
        if (pthread_cond_init(&m_cond, NULL) != 0)
        {
            throw std::exception();
        }
    }

    ~Cond()
    {
        pthread_cond_destroy(&m_cond);
    }

    // 线程缺乏所需资源，进入等待状态。调用函数后线程会释放锁并阻塞，等待条件变量通知它解除阻塞，解除阻塞后再重新加锁
    bool Wait(pthread_mutex_t *mutex)
    {
        return pthread_cond_wait(&m_cond, mutex) == 0;
    }

    // 线程进入阻塞状态，直到指定时间结束
    bool Timedwait(pthread_mutex_t *mutex, const struct timespec t)
    {
        return pthread_cond_timedwait(&m_cond, mutex, &t) == 0;
    }

    // 唤醒一个或多个正在等待的线程
    bool Signal()
    {
        return pthread_cond_signal(&m_cond) == 0;
    }

    // 唤醒所有正在等待的线程
    bool Broadcast()
    {
        return pthread_cond_broadcast(&m_cond) == 0;
    }

private:
    pthread_cond_t m_cond;
};

// 信号量类
class Sem
{
public:
    Sem()
    {
        if (sem_init(&m_sem, 0, 0) != 0)
        {
            throw std::exception();
        }
    }

    Sem(int num)
    {
        if (sem_init(&m_sem, 0, num) != 0)
        {
            throw std::exception();
        }
    }

    ~Sem()
    {
        sem_destroy(&m_sem);
    }

    // 对信号量加锁，调用一次信号量的值 -1。若值为 0 ，就阻塞
    bool Wait()
    {
        return sem_wait(&m_sem) == 0;
    }

    // 对信号量解锁，调用一次信号量的值 +1
    bool Post()
    {
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;
};

#endif