#include "http_connection.h"

int HttpConnection::m_http_connection_numner = 0;
bool HttpConnection::is_ET_mode;
std::string HttpConnection::m_resource_dir;

HttpConnection::HttpConnection() 
{ 
    m_fd = -1;
    m_address = { 0 };
    is_close = true;
};

HttpConnection::~HttpConnection() 
{ 
    Close(); 
};

void HttpConnection::Initialization(int fd, const sockaddr_in& addr) 
{
    assert(fd > 0);
    m_http_connection_numner++;
    m_address = addr;
    m_fd = fd;
    is_close = false;
}

void HttpConnection::Close() 
{
    if(is_close == false)
    {
        is_close = true; 
        m_http_connection_numner--;
        close(m_fd);
    }
}

int HttpConnection::GetFd() const 
{
    return m_fd;
};

struct sockaddr_in HttpConnection::GetAddress() const 
{
    return m_address;
}

int HttpConnection::Read(int &error_num) {
    int len = -1;
    return len;
}

int HttpConnection::Write(int &error_num) 
{
    int len = -1;
    return len;
}

bool HttpConnection::Process() 
{
    return true;
}

bool HttpConnection::IsKeepAlive() const
{
    return is_close;
}

int HttpConnection::GetToWriteBytes() const
{
    return m_fd;
}