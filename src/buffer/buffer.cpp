#include "buffer.h"

Buffer::Buffer(int init_buffer_size) : buffer_(init_buffer_size), 
                read_pos_(0), write_pos_(0) {}

ssize_t Buffer::ReadFd(int fd, int &error_num)
{
    // 为了确保可以将所有数据都读完，当缓冲区满后，剩余字节先保存到临时的 buf 中
    char buf[65535]; 
    struct iovec iov[2];
    const size_t writeable_size = WritableBytes();
    // 调用 readv 分散读来保证数据全部读完
    iov[0].iov_base = GetWritePtr();
    iov[0].iov_len = writeable_size;
    iov[1].iov_base = buf;
    iov[1].iov_len = sizeof(buf);

    ssize_t len = readv(fd, iov, 2);
    if (len < 0)
    {
        error_num = errno;
    }
    else if (static_cast<size_t>(len) <= writeable_size)
    {
        // 当前缓冲区就可以保存所有数据
        UpdateWritePos(static_cast<size_t>(len));
    }
    else
    {
        // 装不下，先移动写指针，再将 buf 中的数据追加到缓冲区
        write_pos_ = buffer_.size(); 
        Append(buf, static_cast<size_t>(len) - writeable_size);
    }
    return len;
}

ssize_t Buffer::WriteFd(int fd, int &error_num)
{
    size_t need_write_size = ReadableBytes();
    ssize_t len = write(fd, GetReadPtr(), need_write_size);
    if (len < 0)
    {
        error_num = errno;
    }
    else
    {
        Retrieve(len);
    }
    return len;
}

void Buffer::Append(const char *str, size_t len)
{
    assert(str);
    EnsureWriteable(len);
    std::copy(str, str + len, GetWritePtr());
    UpdateWritePos(len);
}

void Buffer::Append(const std::string &str)
{
    Append(str.c_str(), str.length());
}

void Buffer::Append(const void *data, size_t len)
{
    assert(data);
    Append(static_cast<const char *>(data), len);
}

void Buffer::Append(const Buffer &buff)
{
    Append(buff.GetReadPtr(), buff.ReadableBytes());
}

void Buffer::Retrieve(size_t len)
{
    assert(len <= ReadableBytes());
    read_pos_ += len;
}

void Buffer::RetrieveUntil(const char *end)
{
    assert(end >= GetReadPtr() && end <= GetWritePtr());
    Retrieve(end - GetReadPtr());
}

void Buffer::RetrieveAll()
{
    std::fill(buffer_.begin(), buffer_.end(), 0);
    read_pos_ = 0;
    write_pos_ = 0;
}

std::string Buffer::RetrieveAllToStr()
{
    std::string str(GetReadPtr(), ReadableBytes());
    RetrieveAll();
    return str;
}

void Buffer::EnsureWriteable(size_t len)
{
    if (WritableBytes() < len)
    {
        ExpandSize(len);
    }
    assert(WritableBytes() >= len);
}

void Buffer::ExpandSize(size_t len)
{
    // 当前已读数据区域的空间和仍能写数据的空间可以保存 len 字节的数据
    if (WritableBytes() + HasReadBytes() >= len)
    {
        size_t need_read_size = ReadableBytes();
        std::copy(GetReadPtr(), GetWritePtr(), BeginPtr());
        read_pos_ = 0;
        write_pos_ = need_read_size;
        assert(need_read_size == ReadableBytes());
    }
    else
    {
        buffer_.resize(write_pos_ + len + 1);
    }
}
