#include "heap_timer.h"

void HeapTimer::Add(int id, int timeout, const TimeoutCallback &func)
{
    assert(id >= 0);
    if (index_.count(id))
    {
        // 已有节点，修改值并调整堆
        heap_[index_[id]].func = func;
        AdjustTime(id, timeout);
    }
    else
    {
        // 添加新节点，向上调整
        size_t index = heap_.size();
        index_[id] = index;
        heap_.push_back({id, Clock::now() + static_cast<MS>(timeout), func});
        HeapifyUp(index);
    }
}

void HeapTimer::AdjustTime(int id, int timeout)
{
    assert(!heap_.empty() && index_.count(id) > 0);
    size_t index = index_[id];
    heap_[index].close_time = Clock::now() + static_cast<MS>(timeout);
    HeapifyDown(index);
    HeapifyUp(index);
}

void HeapTimer::ClearTimeoutNode()
{
    if (heap_.empty())
    {return;}
    while (!heap_.empty())
    {
        TimerNode node = heap_.front();
        if (std::chrono::duration_cast<MS>(node.close_time - Clock::now()).count() > 0)
        {
            break; // 最早的节点未超时
        }
        node.func();
        PopFront();
    }
}

void HeapTimer::PopFront()
{
    assert(!heap_.empty());
    Delete(0);
}

int HeapTimer::GetNextTimeout()
{
    ClearTimeoutNode();
    if (heap_.empty()) return -1;
    int res = std::chrono::duration_cast<MS>(heap_.front().close_time - Clock::now()).count();
    return std::max(res, 0);
}

void HeapTimer::Clear()
{
    index_.clear();
    heap_.clear();
}

void HeapTimer::HeapifyUp(size_t index)
{
    assert(index >= 0 && index < heap_.size());
    int parent = (index - 1) / 2; // 从 0 开始的
    while (index > 0 && heap_[index] < heap_[parent])
    {
        SwapNode(index, parent);
        index = parent;
        parent = (index - 1) / 2;
    }
}

bool HeapTimer::HeapifyDown(size_t index)
{
    assert(index >= 0 && index < heap_.size());
    size_t i = index;
    size_t size = heap_.size(), small_index = index;
    while (index < size) 
    {
        small_index = (2 * index + 1 < size && heap_[2 * index + 1] < heap_[small_index]) ? 2 * index + 1 : small_index;
        small_index = (2 * index + 2 < size && heap_[2 * index + 2] < heap_[small_index]) ? 2 * index + 2 : small_index;
        if (small_index == index) 
        {
            break;
        }
        SwapNode(index, small_index);
        index = small_index;
    }
    return index > i;
}

void HeapTimer::SwapNode(size_t index1, size_t index2)
{
    assert(index1 >= 0 && index1 < heap_.size());
    assert(index2 >= 0 && index2 < heap_.size());
    std::swap(heap_[index1], heap_[index2]);
    // 更改节点 id 对应的下标
    index_[heap_[index1].id] = index1;
    index_[heap_[index2].id] = index2;
}

void HeapTimer::Delete(size_t index) 
{
    assert(!heap_.empty() && index >= 0 && index < heap_.size());
    SwapNode(index, heap_.size() - 1);
    index_.erase(heap_.back().id);
    heap_.pop_back();
    if (!heap_.empty())
    {
        HeapifyDown(index);
    }
}