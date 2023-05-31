#ifndef HTTP_CONNECTION_H
#define HTTP_CONNECTION_H

#include <string>
#include <cassert>
#include <unistd.h>
#include <arpa/inet.h>


class HttpConnection
{
public:
    HttpConnection();
    ~HttpConnection();
    void Initialization(int fd, const struct sockaddr_in &addr);

    int GetFd() const;
    struct sockaddr_in GetAddress() const;
    void Close();

    int Read(int &error_num);

    int Write(int &error_num);

    int GetToWriteBytes() const;

    bool IsKeepAlive() const;

    bool Process();

    static int m_http_connection_numner;
    static std::string m_resource_dir;
    static bool is_ET_mode;
private:
    int m_fd;
    struct sockaddr_in m_address;
    bool is_close;
};

#endif