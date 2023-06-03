#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <vector>
#include <unordered_map>
#include <ctime>
#include <algorithm>
#include <functional>
#include <chrono>
#include <cassert>
#include <iostream>

typedef std::function<void()> TimeoutCallback;
typedef std::chrono::high_resolution_clock Clock; // 高精度时钟，它提供了纳秒级别的计时精度
typedef Clock::time_point TimePoint;              // 表示时间点的类型
typedef std::chrono::milliseconds MS;             // 表示毫秒的类型

/*
Clock::time_point start_tp = std::chrono::high_resolution_clock::now(); // 获取当前时间
Clock::time_point end_tp = std::chrono::high_resolution_clock::now(); // 获取当前时间
std::chrono::milliseconds duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_tp - start_tp); // 转换为ms
*/

// 节点结构体，保存节点id，关闭时间和超时处理函数
struct TimerNode
{
    int id;
    TimePoint close_time;
    TimeoutCallback func;
    bool operator<(const TimerNode &t)
    {
        return close_time < t.close_time;
    }
};

class HeapTimer
{
public:
    HeapTimer()
    {
        heap_.reserve(64);
    }

    ~HeapTimer()
    {
        Clear();
    }

    // 添加一个节点（fd 为 id）。若存在则修改节点；否则插入节点
    void Add(int id, int timeout, const TimeoutCallback &func);

    // 修改节点 id 对应的超时时间
    void AdjustTime(int id, int timeout);

    // 清除超时结点，调用它们的回调函数
    void ClearTimeoutNode();

    // 删掉第一个节点，即最久未活动的节点
    void PopFront();

    // 获取最早的未超时节点到超时需要的时间（ms）
    int GetNextTimeout();

    // 清空数据
    void Clear();

private:
    // 向上调整小根堆
    void HeapifyUp(size_t index);

    // 向下调整小根堆
    bool HeapifyDown(size_t index);

    // 交换 heap_ 中下标为 index1 和 index2 的节点，修改 index_
    void SwapNode(size_t index1, size_t index2);

    // 从小根堆中删除一个节点，并将其 id 从 index_ 中删除
    void Delete(size_t index);

    std::vector<TimerNode> heap_;

    std::unordered_map<int, size_t> index_; // 保存节点 id 到数组下标 index 的映射
};

#endif

