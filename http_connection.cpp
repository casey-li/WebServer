#include "http_connection.h"

// 定义 HTTP 响应的状态信息
const char* success_200_title = "OK";               // 正常状态，表示请求成功
const char* error_400_title = "Bad Request";        // 语义有误，当前请求无法被服务器理解
const char* error_400_data = "Your request has syntax errors!\n";
const char* error_403_title = "Forbidden";          // 服务器拒绝执行请求
const char* error_403_data = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Found";          // 请求失败
const char* error_404_data = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";     // 服务器出现问题
const char* error_500_data = "The server encountered a problem and was unable to process the request.\n";

// 请求的资源的根目录
const std::string resources_dir = "/home/casey/niuke/webserver/resources";

void SetNonblocking(int fd)
{
    int flag = fcntl(fd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flag);
}

// 向epoll中添加需要监听的文件描述符
void AddFd(int epollfd, int fd, bool one_shot) 
{
    epoll_event event;
    event.data.fd = fd;
    // 之前一方断开后，需要检测recv的返回值来确定对方是否断开；可以用事件 EPOLLRDHUP 直接表示该情况
    // EPOLLRDHUP 表示对端关闭连接或者关闭写操作; EPOLLHUP 表示连接发生挂起（可能是由于对端关闭了读操作）或关闭; EPOLLERR 表示错误事件，发生错误
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR; 
    // 防止同一个通信被不同的线程处理
    if(one_shot) 
    {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    // 设置文件描述符非阻塞
    SetNonblocking(fd);  
}

// 从epoll对象中删除文件描述符
void RemoveFd(int epoll_fd, int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

// 修改文件描述符，重置socket上的EPOLLONESHOT事件，以确保下一次可读时，EPOLLIN事件能被触发
void ModifyFd(int epoll_fd, int fd, int ev)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLRDHUP | EPOLLHUP | EPOLLERR | EPOLLONESHOT;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
}

// 所有socket上的事件都被注册到同一个epoll内核事件中，所以设置成静态的
int HttpConnection::m_epoll_fd = -1;
// 所有的客户数
int HttpConnection::m_connection_number = 0;

// 初始化连接，设置端口复用，初始化套接字地址
void HttpConnection::Initialization(const int &fd, const struct sockaddr_in &addr)
{
    m_socket_fd = fd;
    memcpy(&m_address, &addr, sizeof(addr));
    // 设置端口复用
    int reuse = 1;
    setsockopt(m_socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    AddFd(m_epoll_fd, fd, true);
    ++m_connection_number;
    Initialization();
}

// 初始化数据成员，如各种处理数据的下标，缓冲区
void HttpConnection::Initialization()
{
    m_parse_status = PARSE_REQUEST_LINE; // 初始状态为解析请求行
    m_read_index = 0;
    m_checked_index = 0;
    m_parse_index = 0;
    m_method = GET;
    m_url = " ";
    m_version = " ";
    m_host = " ";
    m_linger = false;
    m_request_contnent_length = 0;

    m_iovec_count = 0;
    m_write_index = 0;
    m_already_send_bytes = 0;
    m_need_send_bytes = 0;

    memset(m_read_buf, 0, sizeof(m_read_buf));
    memset(m_request_dir, 0, sizeof(m_request_dir));
    memset(m_write_buf, 0, sizeof(m_write_buf));
}

// 当检测到连接发生错误时关闭连接
void HttpConnection::CloseConnection()
{
    // 设置epoll取消监听，减少连接的用户数
    if (m_socket_fd != -1)
    {
        RemoveFd(m_epoll_fd, m_socket_fd);
        --m_connection_number;
        m_socket_fd = -1;
        memset(&m_address, 0, sizeof(m_address));
    }
}

// 循环读取数据，直到无数据可读或者对方关闭连接
bool HttpConnection::Read()
{
    if (m_read_index >= READ_BUFFER_SIZE)
    {
        return false;
    }
    int read_bytes = 0;
    while (true)
    {
        // 从 m_read_buf + m_read_index 处开始保存数据，大小是 READ_BUFFER_SIZE - m_read_index
        read_bytes = recv(m_socket_fd, m_read_buf + m_read_index, READ_BUFFER_SIZE - m_read_index, 0);
        if (read_bytes == -1)
        {
            // 非阻塞 I/O 操作时，若资源暂时不可用(没有数据可读)会返回 EAGAIN 或 EWOULDBLOCK (二者等价，不同 OS 返回结果)
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break; 
            }
            return false;
        }
        // 对方关闭连接
        else if (read_bytes == 0) 
        {
            return false; 
        }
        m_read_index += read_bytes;
    }
    // printf("recv info : %s \n", m_read_buf);
    return true;
}

bool HttpConnection::Write()
{
    if (m_need_send_bytes == 0)
    {
        // 没有需要发送的数据，本次响应结束，epoll 监听 EPOLLIN
        ModifyFd(m_epoll_fd, m_socket_fd, EPOLLIN);
        Initialization();
        return true;
    }
    int res = 0;
    while (1)
    {
        res = writev(m_socket_fd, m_iovec, m_iovec_count);
        if (res <= -1)
        {
            // 如果TCP写缓冲没有空间，则等待下一轮EPOLLOUT事件，虽然在此期间，
            // 服务器无法立即接收到同一客户的下一个请求，但可以保证连接的完整性。
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                ModifyFd(m_epoll_fd, m_socket_fd, EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }
        m_need_send_bytes -= res;
        m_already_send_bytes += res;
        
        // 每发送完一次数据，修改一次缓冲区的大小以及起始位置
        if (m_already_send_bytes >= m_write_index)
        {
            m_iovec[0].iov_len = 0;
            m_iovec[1].iov_base = m_file_address + (m_already_send_bytes - m_write_index);
            m_iovec[1].iov_len = m_need_send_bytes;
        }
        else
        {
            m_iovec[0].iov_base = m_write_buf + m_already_send_bytes;
            m_iovec[0].iov_len = m_iovec[0].iov_len - res;
        }

        // 没有数据需要继续发送了
        if (m_need_send_bytes <= 0)
        {
            unmap();
            ModifyFd(m_epoll_fd, m_socket_fd, EPOLLIN);
            if (m_linger)
            {
                Initialization();
                return true;
            }
            else
            {
                return false;
            }
        }
    }
    return true;
}

// 解析 HTTP 请求，有限状态机
HttpConnection::HTTP_STATUS HttpConnection::ProcessRead()
{
    LINE_STATUS line_status = ON_LINE_PARSE;
    HTTP_STATUS parse_result = ON_REQUEST;
    // HTTP 请求行和请求头以 '\r''\n' 结尾，用 ParseLine() 划分
    while ((m_parse_status == PARSE_CONTENT && line_status == LINE_PARSE_OK) 
            || (line_status = ParseLine()) == LINE_PARSE_OK)
    {
        char *text = GetLine(); //获取一行数据
        m_parse_index = m_checked_index;
        // printf("get HTTP request line data: %s\n", text);
        // 有限状态机，根据当前 HTTP 的解析状态进行相应的处理
        switch (m_parse_status)
        {
            case PARSE_REQUEST_LINE:
            {
                parse_result = ParseRequestLine(text);
                if (parse_result == SYNTAX_ERROR) // 出现语法错误
                {
                    return SYNTAX_ERROR;
                }
                break;
            }
            case PARSE_HEADER:
            {
                parse_result = ParseHeader(text);
                if (parse_result == SYNTAX_ERROR) // 出现语法错误
                {
                    return SYNTAX_ERROR;
                }
                else if (parse_result == GET_REQUEST) // 获取了一个完整请求
                {
                    return DoRequest();
                }
                break;
            }
            case PARSE_CONTENT:
            {
                parse_result = ParseContent(text);
                if (parse_result == GET_REQUEST)
                {
                    return DoRequest();
                }
                line_status = ON_LINE_PARSE;
                break;
            }
            default:
            {
                return INTERNAL_ERROR;
            }
        }
    }
    return ON_REQUEST;
}

// 解析一行，判断依据为: \r\n结尾
// 调用前, m_checked_index == m_parse_index ;调用过后 m_parse_index 不变, m_checked_index 到下一行起始位置
HttpConnection::LINE_STATUS HttpConnection::ParseLine()
{
    char tmp = '0';
    while (m_checked_index < m_read_index)
    {
        tmp = m_read_buf[m_checked_index];
        if (tmp == '\r')
        {
            if (m_checked_index + 1 == m_read_index)
            {
                return ON_LINE_PARSE;
            }
            // 置位 '\0' 表示行结束，之后就可以直接读取一行
            else if (m_read_buf[m_checked_index + 1] == '\n')
            {
                m_read_buf[m_checked_index++] = '\0';
                m_read_buf[m_checked_index++] = '\0';
                return LINE_PARSE_OK;
            }
            return LINE_PARSE_ERROR;
        }
        else if (tmp == '\n')
        {
            if (m_checked_index > 1 && m_read_buf[m_checked_index - 1] == '\r')
            {
                m_read_buf[m_checked_index - 1] = '\0';
                m_read_buf[m_checked_index++] = '\0';
                return LINE_PARSE_OK;
            }
            return LINE_PARSE_ERROR;
        }
        ++m_checked_index;
    }
    return ON_LINE_PARSE;
}

// 解析 Http 请求行内容，包括 方法，URL 和 HTTP版本，它们以空格划分
HttpConnection::HTTP_STATUS HttpConnection::ParseRequestLine(char *text)
{
    std::string info(text);
    std::stringstream ss(info);
    std::string tmp;
    auto ToUpper = [](std::string &str)
    {
        for (char &c : str)
        {
            if (isalpha(c))
            {
                c = toupper(c);
            }
        }
    };
    // 解析方法
    std::getline(ss, tmp, ' ');
    ToUpper(tmp);
    if (tmp != "GET")
    {
        return SYNTAX_ERROR;
    }
    m_method = GET;
    // 解析 URL，获取文件如 http://192.168.190.128/index.html 只要 index.html
    std::getline(ss, tmp, ' ');
    size_t it = 0;
    if (tmp.substr(0, 7) == "http://")
    {
        it = tmp.find('/', 7);
    }
    else
    {
        it = tmp.find('/', 0);
    }
    if (it == std::string::npos)
    {
        return SYNTAX_ERROR;
    }
    m_url = tmp.substr(it + 1);
    // 解析 HTTP 协议版本号，只支持 HTTP/1.1
    std::getline(ss, tmp, ' ');
    ToUpper(tmp);
    if (tmp != "HTTP/1.1")
    {
        return SYNTAX_ERROR;
    }
    m_version = tmp;
    printf("parse result :\nmethod: %d, url: %s, version: %s \n", m_method, m_url.c_str(), m_version.c_str());
    m_parse_status = PARSE_HEADER; // 请求行解析完毕，当前解析状态变为解析请求头部
    return ON_REQUEST;
}

// 解析 HTTP 请求头部信息
HttpConnection::HTTP_STATUS HttpConnection::ParseHeader(char *text)
{
    // 遇到空行，表示头部字段解析完毕
    if (text[0] == '\0')
    {
        // 如果 HTTP 请求有消息体，则还需读取 m_request_content_length 字节的消息体
        // 修改解析状态为 PARSE_CONTENT
        if (m_request_contnent_length)
        {
            m_parse_status = PARSE_CONTENT;
            return ON_REQUEST;
        }
        return GET_REQUEST; // 已经得到了一个完整请求
    }
    // 处理 Connection 头部字段  Connection: keep-alive
    else if (strncasecmp(text, "Connection:", 11) == 0) // 忽略大小写的比较
    {
        text += 11; // 跳过 "Connection:"
        text += strspn( text, " \t" ); // 继续跳过 空格、制表符
        if (strncasecmp(text, "keep-alive", 10) == 0)
        {
            m_linger = true;
        }
        printf("parse Connection: %s\n", m_linger ? "keep-alive" : "close");
    }
    // 处理 Content-Length 头部字段
    else if (strncasecmp(text, "Content-Length:", 15) == 0)
    {
        text += 15;
        text += strspn(text, " \t");
        m_request_contnent_length = atol(text);
        printf("parse Content-Length: %d\n", m_request_contnent_length);
    }
    // 处理 Host 头部字段
    else if (strncasecmp(text, "Host:", 5) == 0)
    {
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
        printf("parse Host: %s\n", m_host.c_str());
    }
    else
    {
        // printf("oop! unknown header! : %s \n", text);
        ;
    }
    return ON_REQUEST;
}

// 目前只是判断它是否被完整的读入，并不是真正解析HTTP请求的消息体
HttpConnection::HTTP_STATUS HttpConnection::ParseContent(char *text)
{
    if (m_read_index >= (m_request_contnent_length + m_checked_index))
    {
        text[m_read_index] = '\0';
        return GET_REQUEST;
    }
    return ON_REQUEST;
}

// 当解析到完整请求后，调用处理请求函数，分析目标文件的属性
// 若目标文件存在，对所有用户可读并且不是目录，就将其映射到内存地址
// m_file_address 处，方便写回，并设置状态为获取文件成功
HttpConnection::HTTP_STATUS HttpConnection::DoRequest()
{
    std::string file_path = resources_dir + "/" + m_url;
    if (file_path.size() >= MAX_DIR_LENGTH)
    {
        return NO_RESOURCE;
    }
    strcpy(m_request_dir, file_path.c_str());
    // printf ("request file path is : %s\n", m_request_dir);
    // 获取 m_request_dir 文件的相关的状态信息
    if (stat(m_request_dir, &m_file_stat) == -1)
    {
        return NO_RESOURCE;
    }
    // 判断访问权限, S_IROTH, S_IWOTH, S_IXOTH 分别表示其他人能否读, 写, 执行
    if (!(m_file_stat.st_mode & S_IROTH))
    {
        return FORBIDDEN_ACCESS;
    }
    // 判断是否是目录
    if (S_ISDIR(m_file_stat.st_mode))
    {
        return SYNTAX_ERROR;
    }
    // 以只读方式打开文件
    int fd = open(m_request_dir, O_RDONLY);
    // 创建内存映射
    void *memory_address = mmap(NULL, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (memory_address == MAP_FAILED)
    {
        return INTERNAL_ERROR;
    }
    m_file_address = static_cast<char *>(memory_address);
    close(fd); // 关闭创建内存映射区的 fd，不会对 mmap() 映射产生影响
    return GET_FILE;
}

// 对内存映射区执行 munmap 操作
void HttpConnection::unmap()
{
    if (m_file_address)
    {
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = nullptr;
    }
}

// 根据 HTTP 解析的结果，生成不同的 HTTP 响应报文（包括 状态行，响应头部 和 响应正文）
bool HttpConnection::ProcessWrite(HTTP_STATUS parse_status)
{
    switch(parse_status)
    {
        case SYNTAX_ERROR:      // 请求报文语法错误
        {
            // printf("%s\n", "syntax error!");
            if (!AddStatusLine(400, error_400_title) || !AddRespondHeader(strlen(error_400_data))
                || !AddContent(error_400_data))
            {
                return false;
            }
            break;
        }
        case NO_RESOURCE:       // 服务器没有客户请求的资源
        {
            // printf("%s\n", "no resource!");
            if (!AddStatusLine(404, error_404_title) || !AddRespondHeader(strlen(error_404_data))
                || !AddContent(error_404_data))
            {
                return false;
            }
            break;
        }
        case FORBIDDEN_ACCESS:  // 客户访问了服务器上没有权限访问的文件
        {
            // printf("%s\n", "forbidden access!");
            if (!AddStatusLine(403, error_403_title) || !AddRespondHeader(strlen(error_403_data))
                || !AddContent(error_403_data))
            {
                return false;
            }
            break;
        }
        case GET_FILE:          // 获取到目标文件，此时有两个内存区需要写回给客户
        {
            // printf("%s\n", "get file success!");
            if (!AddStatusLine(200, success_200_title) || !AddRespondHeader(m_file_stat.st_size))
            {
                return false;
            }
            m_iovec[0].iov_base = m_write_buf;
            m_iovec[0].iov_len = m_write_index;
            m_iovec[1].iov_base = m_file_address;
            m_iovec[1].iov_len = m_file_stat.st_size;
            m_iovec_count = 2;
            m_need_send_bytes = m_write_index + m_file_stat.st_size;
            return true;
        }
        case INTERNAL_ERROR:    // 服务器内部发生错误
        {
            // printf("%s\n", "internal error!");
            if (!AddStatusLine(500, error_500_title) || !AddRespondHeader(strlen(error_500_data))
                || !AddContent(error_500_data))
            {
                return false;
            }
            break;
        }
        default:
            // printf("%s\n", "default!");
            return false;
    }
    // 未获取到文件
    m_iovec[ 0 ].iov_base = m_write_buf;
    m_iovec[ 0 ].iov_len = m_write_index;
    m_iovec_count = 1;
    m_need_send_bytes = m_write_index;
    return true;
}

// 向缓冲区中写入 fromat 格式的数据
bool HttpConnection::AddResponse(const char *format, ...)
{
    if (m_write_index >= WRITE_BUFFER_SIZE)
    {
        return false;
    }
    va_list args;
    va_start(args, format);
    // 向一个字符串缓冲区打印格式化字符串，因为vsnprintf还要在结果的末尾追加\0，所以大小还得 - 1
    // int vsnprintf (char * sbuf, size_t n, const char * format, va_list arg);
    int len = vsnprintf(m_write_buf + m_write_index, WRITE_BUFFER_SIZE -1 - m_write_index, format, args);
    if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_index))
    {
        return false;
    }
    m_write_index += len;
    va_end(args);
    return true;
}

// 添加状态行信息，包括 协议版本, 状态码 和 状态码描述, 中间以空格隔开，结尾添加 "\r\n"
bool HttpConnection::AddStatusLine(int status, const char *status_title)
{
    // printf("%s\n", "add status line");
    return AddResponse("%s %d %s\r\n", "HTTP/1.1", status, status_title);
}

// 添加响应头部信息，包括响应正文长度，响应正文种类，是否持久化连接和空行，每个信息都以 \r\n 结尾
bool HttpConnection::AddRespondHeader(int content_length)
{
    if (!AddContentLength(content_length) || !AddContentType()
        || !AddLinger() || !AddBlankLine())
    {
        return false;
    }
    return true;
}

// 在响应头部中添加响应正文长度信息
bool HttpConnection::AddContentLength(int content_length)
{
    return AddResponse("Content-Length: %d\r\n", content_length);
}

// 在响应头部中添加响应正文种类信息
bool HttpConnection::AddContentType()
{
    return AddResponse("Content-Type: %s\r\n", "text/html");
}

// 在响应头部中添加是否持久化连接信息
bool HttpConnection::AddLinger()
{
    return AddResponse("Connection: %s\r\n", m_linger ? "keep-alive" : "close");
}

// 在响应头部中添加空行
bool HttpConnection::AddBlankLine()
{
    return AddResponse("%s", "\r\n");
}

// 添加响应正文信息
bool HttpConnection::AddContent(const char *content)
{
    return AddResponse("%s", content);
}

// 由线程池中的工作线程调用，即处理 HTTP 请求的入口函数
void HttpConnection::Process()
{
    // 解析 HTTP 请求
    HTTP_STATUS parse_status = ProcessRead();
    if (parse_status == ON_REQUEST)
    {
        // 此时读取的请求信息不完整，需要继续读取客户数据，设置 epoll 的 EPOLLIN
        ModifyFd(m_epoll_fd, m_socket_fd, EPOLLIN);
        return;
    }
    // 生成 HTTP 响应报文
    bool write_result = ProcessWrite(parse_status);
    if (!write_result)
    {
        CloseConnection();
    }
    ModifyFd(m_epoll_fd, m_socket_fd, EPOLLOUT);
}
