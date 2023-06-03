#ifndef BLOCK_DEQUE_H
#define BLOCK_DEQUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>

template<typename T>
class BlockDeque
{
public:
    explicit BlockDeque(size_t max_capacity = 1024);

    ~BlockDeque();
    
    // 若放不下了阻塞生产者，添加元素后通知消费者取数据
    void PushBack(const T &item);

    void PushFront(const T &item);

    // 从队头弹出元素，无元素则阻塞；取出元素后通知生产者
    bool Pop(T &item);

    // 同上，不过设置了消费者的阻塞时间
    bool Pop(T &item, int timeout);

    bool Empty();

    bool Full();
    
    void Clear();

    void Close();

    // 唤醒一个等待中的消费者线程
    void Flush();

    T Front();

    T Back();

private:

    size_t capacity_;

    bool is_close_;

    std::deque<T> dq_;

    std::mutex mtx_;

    std::condition_variable cond_consumer_, cond_producer_;
};

template <typename T>
BlockDeque<T>::BlockDeque(size_t max_capacity) : capacity_(max_capacity), is_close_(false)
{
    assert(max_capacity > 0);
}

template<typename T>
BlockDeque<T>::~BlockDeque()
{
    Close();
}

template<typename T>
void BlockDeque<T>::PushBack(const T &item)
{
    std::unique_lock<std::mutex> locker(mtx_);
    // 当放不下了以后阻塞生产者，等待消费者取数据
    while (dq_.size() >= capacity_)
    {
        cond_producer_.wait(locker);
    }
    // 生产者放了数据以后，通知一个消费者去取数据
    dq_.push_back(item);
    cond_consumer_.notify_one();
}

template<typename T>
void BlockDeque<T>::PushFront(const T &item)
{
    std::unique_lock<std::mutex> locker(mtx_);
    while (dq_.size() >= capacity_)
    {
        cond_producer_.wait(locker);
    }
    dq_.push_front(item);
    cond_consumer_.notify_one();
}

template<typename T>
bool BlockDeque<T>::Pop(T &item)
{
    std::unique_lock<std::mutex> locker(mtx_);
    while (dq_.empty())
    {
        cond_consumer_.wait(locker);
        if (is_close_)
        {
            return false;
        }
    }
    item = dq_.front();
    dq_.pop_front();
    cond_producer_.notify_one();
    return true;
}

template<typename T>
bool BlockDeque<T>::Pop(T &item, int timeout)
{
    std::unique_lock<std::mutex> locker(mtx_);
    while (dq_.empty())
    {
        // 若阻塞时间超过了设置的时间仍未收到通知则返回 std::cv_status::timeout
        if (cond_consumer_.wait_for(locker, static_cast<std::chrono::seconds>(timeout))
            == std::cv_status::timeout)
        {
            return false;
        }
        if (is_close_)
        {
            return false;
        }
    }
    item = dq_.front();
    dq_.pop_front();
    cond_producer_.notify_one();
    return true;
}

template<typename T>
bool BlockDeque<T>::Empty()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return dq_.empty();
}

template<typename T>
bool BlockDeque<T>::Full()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return dq_.size() >= capacity_;
}

template<typename T>
void BlockDeque<T>::Clear()
{
    std::lock_guard<std::mutex> locker(mtx_);
    dq_.clear();
}

template<typename T>
void BlockDeque<T>::Close()
{
    {
        std::lock_guard<std::mutex> locker(mtx_);
        dq_.clear();
        is_close_ = true;
    }
    cond_producer_.notify_all();
    cond_consumer_.notify_all();
}

template<typename T>
void BlockDeque<T>::Flush()
{
    cond_consumer_.notify_one();
}

template<typename T>
T BlockDeque<T>::Front()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return dq_.front();
}

template<typename T>
T BlockDeque<T>::Back()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return dq_.back();
}

#endif