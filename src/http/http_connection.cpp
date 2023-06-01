#include "http_connection.h"

std::atomic<int> HttpConnection::http_connection_numner_;
std::string HttpConnection::resource_dir_;
bool HttpConnection::is_ET_mode_;

HttpConnection::HttpConnection() : fd_(-1), is_close_(true), iov_num_(0) 
{ 
    address_ = { 0 };
};

HttpConnection::~HttpConnection() 
{ 
    Close(); 
};

void HttpConnection::Initialization(int fd, const sockaddr_in& addr) 
{
    assert(fd > 0);
    fd_ = fd;
    address_ = addr;
    is_close_ = false;
    read_buf_.RetrieveAll();
    write_buf_.RetrieveAll();
    http_connection_numner_++;
    // 打印日志
}

ssize_t HttpConnection::Read(int &error_num)
{
    ssize_t len = -1;
    // 若为 ET 模式的话，需要一直读直到所有数据读完
    do
    {
        len = read_buf_.ReadFd(fd_, error_num);
        if (len <= 0)
        {
            break;
        }
    } while (is_ET_mode_);
    return len;
}

bool HttpConnection::Process() 
{
    request_.Initialization();
    if (read_buf_.ReadableBytes() <= 0)
    {
        return false;
    }
    else if (request_.Parse(read_buf_))
    {
        // 打印日志
        response_.Initialization(resource_dir_, request_.GetPath(), request_.GetIsKeepAlive(), 200);
    }
    else
    {
        response_.Initialization(resource_dir_, request_.GetPath(), false, 400);
    }
    response_.MakeResponse(write_buf_);
    // 响应头
    iov_[0].iov_base = write_buf_.GetReadPtr();
    iov_[0].iov_len = write_buf_.ReadableBytes();
    iov_num_ = 1;
    // 请求文件
    if (response_.GetFileSize() > 0 && response_.GetFileAddr())
    {
        iov_[1].iov_base = response_.GetFileAddr();
        iov_[1].iov_len = response_.GetFileSize();
        iov_num_ = 2;
    }
    // 打印日志
    return true;
}

ssize_t HttpConnection::Write(int &error_num)
{
    ssize_t len = -1;
    do
    {
        len = writev(fd_, iov_, iov_num_);
        if (len <= 0)
        {
            error_num = errno;
            break;
        }
        if (GetToWriteBytes() == 0)
        {
            break; // 传输完成
        }
        // 第一块内存缓冲区中的数据写完了，清空第一块缓冲区，更新第二块缓冲区的起始位置和大小
        // 注意 iov_[1].iov_base 是 void *类型的，对无类型指针进行算术运算是不被允许的，因为编译器无法确定运算的单位大小
        else if (static_cast<size_t>(len) > iov_[0].iov_len)
        {
            iov_[1].iov_base = static_cast<char *>(iov_[1].iov_base) + (len - iov_[0].iov_len);
            iov_[1].iov_len -=  (len - iov_[0].iov_len);
            if (iov_[0].iov_len)
            {
                write_buf_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        else // 第一块缓冲区中的数据还未写完，仅更新第一块缓冲区的相关信息
        {
            iov_[0].iov_base = static_cast<char *>(iov_[0].iov_base) + len;
            iov_[0].iov_len -= len;
            write_buf_.Retrieve(len);
        }
    } while (is_ET_mode_ || GetToWriteBytes() > 0);
    return len;   
}

void HttpConnection::Close() 
{
    response_.UnmapFile();
    if(is_close_ == false)
    {
        is_close_ = true; 
        http_connection_numner_--;
        close(fd_);
        // 打印日志
    }
}