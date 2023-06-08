#ifndef HTTP_CONNECTION_H
#define HTTP_CONNECTION_H

#include <string>
#include <cassert>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/uio.h> // struct iovec
#include <atomic>
#include "../buffer/buffer.h"
#include "../log/log.h"
#include "http_request.h"
#include "http_response.h"

class HttpConnection
{
public:

    static std::atomic<int> http_connection_numner_;    // 保存连接的客户端数目，所有实例共享，定义为原子操作

    static std::string resource_dir_;   // 保存资源的存放地址

    static bool is_ET_mode_;            // 保存客户的连接是否为 ET 模式

    HttpConnection();

    ~HttpConnection();

    // 初始化一个连接
    void Initialization(int fd, const struct sockaddr_in &addr);

    // 读请求报文
    ssize_t Read(int &error_num);

    // 让 request_ 解析读到的请求报文并让 response_ 生成响应报文，设置内存缓冲区 iovec 的信息
    bool Process();

    // 将生成的响应报文写回
    ssize_t Write(int &error_num);

    // 关闭连接
    void Close();

    int GetToWriteBytes() const
    {
        return iov_[0].iov_len + iov_[1].iov_len;
    }

    int GetFd() const
    {
        return fd_;
    }

    int GetPort() const
    {
        return ntohs(address_.sin_port);
    }

    // 使用 inet_ntop 得先把地址转换为主机字节序
    std::string GetIP() const
    {
        char ip[16] = {0};
        return std::string(inet_ntop(AF_INET, &address_.sin_addr.s_addr, ip, 16));
    }
    
    struct sockaddr_in GetAddress() const
    {
        return address_;
    }

    // 返回请求报文中的 Connection 信息
    bool IsKeepAlive() const
    {
        return request_.GetIsKeepAlive();
    }

private:
    int fd_;            // 客户端连接的 fd

    struct sockaddr_in address_; // 客户端地址

    bool is_close_;     // 当前连接是否关闭

    int iov_num_;

    struct iovec iov_[2];

    Buffer read_buf_;   // 读缓冲区

    Buffer write_buf_;  // 写缓冲区

    HttpRequest request_;

    HttpResponse response_;
};
#endif