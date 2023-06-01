缓冲区 `buffer` 类：利用标准库容器封装char，实现自动增长的缓冲区

std::atomic<>是C++标准库中提供的模板类，用于在多线程环境中对变量进行原子操作。它提供了一系列的成员函数和操作符，常用的用法包括：

1. 加载和存储操作：

- load()：原子地加载当前值并返回。
- store(value)：原子地存储给定的值。

2. 读-修改-写操作：

- exchange(value)：原子地将当前值设置为给定的值，并返回之前的值。
- fetch_add(value)：原子地将当前值加上给定的值，并返回之前的值。
- fetch_sub(value)：原子地将当前值减去给定的值，并返回之前的值。

3. 比较和交换操作：

- compare_exchange_weak(expected, desired)：如果当前值等于expected，则将其设置为desired，并返回true；否则，将expected更新为当前值，返回false。
- compare_exchange_strong(expected, desired)：与compare_exchange_weak()类似，但是对于比较失败时的重试次数更多。

4. 原子操作符：

- +=：原子地将当前值增加给定的值。
- -= ：原子地将当前值减去给定的值。
- ++：原子地将当前值增加1。
- --：原子地将当前值减去1。