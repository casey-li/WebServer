#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <string>
#include <atomic>
#include <sys/uio.h> // struct iovec
#include <cassert>
#include <unistd.h>

class Buffer
{
public:
    Buffer(int init_buffer_size = 1024);

    ~Buffer() = default;

    // 从文件描述符中读取数据到缓冲区（分散读，若缓冲区保存不下再进行追加）
    ssize_t ReadFd(int fd, int &error_num);

    // 将缓冲区中的数据写入到文件描述符中
    ssize_t WriteFd(int fd, int &error_num);

    // 将长度为 len 的字符序列追加到缓冲区中（大小不够则进行扩容）
    void Append(const char *str, size_t len);

    // 将字符串追加到缓冲区中，调用 void Append(const char *str, size_t len)
    void Append(const std::string &str);

    // 将数据追加到缓冲区中，调用 void Append(const char *str, size_t len)
    void Append(const void *data, size_t len);

    // 将另一个缓冲区读到的数据追加到缓冲区中，调用 void Append(const char *str, size_t len)
    void Append(const Buffer &buff);

    // 读取了 len 字节的数据，修改读位置
    void Retrieve(size_t len);

    // 读取到了 end 位置，修改读位置
    void RetrieveUntil(const char *end);

    // 清空缓冲区，将读写指针重置为初始位置
    void RetrieveAll();

    // 清空缓冲区并返回缓冲区中的数据
    std::string RetrieveAllToStr();

    // 写入了 len 字节的数据，更新写指针的位置
    void UpdateWritePos(size_t len)
    {
        m_write_pos += len;
    }

    // 返回缓冲区中可读数据的指针
    char *GetReadPtr()
    {
        return BeginPtr() + m_read_pos;
    }

    // 返回缓冲区中可读数据的常量指针
    const char *GetReadPtr() const
    {
        return BeginPtr() + m_read_pos;
    }

    // 返回可写位置的指针
    char *GetWritePtr()
    {
        return BeginPtr() + m_write_pos;
    }

    // 返回可写位置的常量指针
    const char *GetWritePtr() const
    {
        return BeginPtr() + m_write_pos;
    }

    // 返回可写的字节数目
    size_t WritableBytes() const
    {
        return m_buffer.size() - m_write_pos;
    }

    // 返回可读的字节数目
    size_t ReadableBytes() const
    {
        return m_write_pos - m_read_pos;
    }

    // 返回已经读取的字节数目，扩容的时候可以利用这部分空间
    size_t HasReadBytes() const
    {
        return m_read_pos;
    }

private:
    // 确保缓冲区仍有大小为 len 的可写空间，如果不够则进行扩容
    void EnsureWriteable(size_t len);

    // 在需要扩容的时候调整缓冲区大小（先利用已读空间，仍不够扩容）
    void ExpandSize(size_t len);

    // 返回指向缓冲区首元素的指针
    char *BeginPtr()
    {
        return &m_buffer.front();
    }

    // 返回指向缓冲区首元素的常量指针
    const char *BeginPtr() const
    {
        return &m_buffer.front();
    }

    std::vector<char> m_buffer;

    // 利用 atomic 实现对变量的原子操作
    // 原子类型对象的主要特点就是从不同线程访问不会导致数据竞争
    std::atomic<size_t> m_read_pos; // 读位置

    std::atomic<size_t> m_write_pos; // 写位置
};

#endif