#ifndef HTTPCONNECTION
#define HTTPCONNECTION

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <cstdio>
#include <cerrno>
#include <string>
#include <sstream>
#include <algorithm>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/uio.h>


// 所有socket事件都注册到同一个epoll内核事件中，并且所有类的实例共享当前连接的用户数，因此将他们设为静态变量
class HttpConnection
{
public:
    static const int READ_BUFFER_SIZE = 2048;   // 读缓冲区的大小
    static const int WRITE_BUFFER_SIZE = 2048;  // 写缓冲区的大小
    static const int MAX_DIR_LENGTH = 200;      // 请求的文件名完整路径的最大长度

public:
    // HTTP请求方法，这里只支持GET
    enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT};
    
    /*
        服务器处理HTTP请求的可能结果，报文解析的结果
        ON_REQUEST          :   请求不完整，需要继续读取客户数据
        GET_REQUEST         :   表示获得了一个完成的客户请求
        SYNTAX_ERROR        :   表示客户请求语法错误
        NO_RESOURCE         :   表示服务器没有资源
        FORBIDDEN_ACCESS    :   表示客户对资源没有足够的访问权限
        GET_FILE            :   文件请求,获取文件成功
        INTERNAL_ERROR      :   表示服务器内部错误
        CLOSED_CONNECTION   :   表示客户端已经关闭连接了
    */  
    enum HTTP_STATUS {ON_REQUEST, GET_REQUEST, SYNTAX_ERROR, NO_RESOURCE, FORBIDDEN_ACCESS, GET_FILE, INTERNAL_ERROR, CLOSED_CONNECTION };
    
    /*
        解析客户端请求时，主状态机的状态
        PARSE_REQUEST_LINE：当前正在分析请求行
        PARSE_HEADER：当前正在分析头部字段
        PARSE_CONTENT：当前正在解析请求体
    */
    enum PARSE_STATUS {PARSE_REQUEST_LINE = 0, PARSE_HEADER, PARSE_CONTENT};
    
    // 从状态机的三种可能状态，即行的读取状态，分别表示
    // 1.读取到一个完整的行 2.行出错 3.行数据尚且不完整
    enum LINE_STATUS {LINE_PARSE_OK = 0, LINE_PARSE_ERROR, ON_LINE_PARSE};


public:
    static int m_epoll_fd;
    static int m_connection_number;

public:
    HttpConnection(){}
    ~HttpConnection(){}
    void Initialization(const int& fd, const sockaddr_in &addr); // 初始化新接受的连接
    void Initialization();          // 初始化数据
    void CloseConnection();         // 关闭连接
    bool Read();                    // 非阻塞读
    bool Write();                   // 非阻塞写
    void Process();                 // 处理客户端的请求

    HTTP_STATUS ProcessRead();      //解析 HTTP 请求并返回解析结果

    // 被 ProcessRead() 调用的函数，用于解析 HTTP 请求
    LINE_STATUS ParseLine();                                // 从整个请求中找到 '\r\n' 标志，划分出一行信息
    HTTP_STATUS ParseRequestLine(char *text);               // 解析请求行内容
    HTTP_STATUS ParseHeader(char *text);                    // 解析请求头部信息
    HTTP_STATUS ParseContent(char *text);                   // 解析请求体
    char * GetLine() {return m_read_buf + m_parse_index;};  // 获取一行信息

    HTTP_STATUS DoRequest();

    // 被 ProcessWrite() 调用的函数，用于填充 HTTP 响应报文
    bool ProcessWrite(HTTP_STATUS parse_status);                //生成 HTTP 响应报文
    bool AddResponse(const char *fromat, ...);                  // 根据给定格式写信息到响应报文
    bool AddStatusLine(int status, const char *status_title);   // 添加状态行信息
    bool AddRespondHeader(int content_length);                  // 添加响应头部信息，包括响应正文长度，响应类型，是否保持连接，空行（每部分都以\r\n结尾）
    bool AddContentLength(int content_length);                  // 添加响应正文长度
    bool AddContentType();                                      // 添加响应内容类型 text/html
    bool AddLinger();                                           // 添加是否持久化连接信息
    bool AddBlankLine();                                        // 添加空行
    bool AddContent(const char*content);                        // 添加响应正文

    void unmap();                                               // 对创建的内存映射区执行 munmap 操作


private:
    int m_socket_fd;                        // 该HTTP连接的socket
    sockaddr_in m_address;                  // 客户端的地址

    char m_read_buf[READ_BUFFER_SIZE];      // 读缓冲区
    int m_read_index;                       // 标识读缓冲区中已经读入的客户端数据的最后一个字节的下一个位置

    PARSE_STATUS m_parse_status;            // 主状态机当前所处的状态，根据它确定下一步动作
    int m_checked_index;                    // 当前正在分析的字节在读缓冲区中的位置
    int m_parse_index;                      // 当前正在解析的行的起始位置，调用GetLine()时使用
    METHOD m_method;                        // 请求方法
    std::string m_url;                      // 客户请求的目标文件的文件名
    std::string m_version;                  // HTTP协议版本号，仅支持 HTTP1.1
    std::string m_host;                     // 主机名
    bool m_linger;                          // HTTP 请求是否要求保持连接
    int m_request_contnent_length;          // HTTP请求的总长度

    char m_request_dir[MAX_DIR_LENGTH];     // 客户请求的文件的路径，为 resources_dir + m_url，resources_dir为资源存放地址
    char *m_file_address;                   // 客户请求的目标文件被mmap到内存中的起始位置
    struct stat m_file_stat;                // 目标文件的状态，用于判断文件是否存在、是否可读、是否为目录、文件大小等信息
    struct iovec m_iovec[2];                // 采用writev来执行写操作，一次将多个缓冲区的数据写入文件描述符，每个结构体记录起始位置和大小
    int m_iovec_count;                      // 被写内存块的数量

    char m_write_buf[WRITE_BUFFER_SIZE];    // 写缓冲区
    int m_write_index;                      // 写缓冲区中待发送的字节数
    int m_already_send_bytes;                 // 保存已经发送的字节数
    int m_need_send_bytes;                    // 保存待发送的字节数
    

};

#endif