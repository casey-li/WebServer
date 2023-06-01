#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include "../buffer/buffer.h"

class HttpRequest
{
public:
    // 解析客户端请求时，主状态机的状态
    enum PARSE_STATUS
    {
        PARSE_REQUEST_LINE = 0, // 当前正在分析请求行
        PARSE_HEADER,           // 当前正在分析头部字段
        PARSE_CONTENT,          // 当前正在解析请求体
        FINSH,                  // 解析完成
    };

    // 服务器处理HTTP请求的可能结果，报文解析的结果
    enum HTTP_STATUS
    {
        ON_REQUEST = 0,    // 请求不完整，需要继续读取客户数据
        GET_REQUEST,       // 表示获得了一个完成的客户请求
        SYNTAX_ERROR,      // 表示客户请求语法错误
        NO_RESOURCE,       // 表示服务器没有资源
        FORBIDDEN_ACCESS,  // 表示客户对资源没有足够的访问权限
        GET_FILE,          // 文件请求,获取文件成功
        INTERNAL_ERROR,    // 表示服务器内部错误
        CLOSED_CONNECTION, // 表示客户端已经关闭连接了
    };

    HttpRequest()
    {
        Initialization();
    }

    ~HttpRequest() = default;

    void Initialization();

    bool Parse(Buffer &buf);

    std::string GetPath() const;

    std::string &GetPath();

    std::string GetMethod();

    std::string GetVersion();

    std::string GetPost(const std::string &key) const;

    std::string GetPost(const char *key) const;

    bool GetIsKeepAlive() const;

private:
    void ParseRequestLine(const std::string &line);

    void ParseHeader(const std::string &line);

    void ParseContent(const std::string &line);

    void ParsePath();

    void ParsePost();

    void ParseFromUrlEncoded();

    PARSE_STATUS parse_status_;

    std::string method_, path_, version_, body_;

    std::unordered_map<std::string, std::string> header_;

    std::unordered_map<std::string, std::string> post_;

    static const std::unordered_set<std::string> DEFAULT_HTML;

    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;

    static int ConverHex(char ch);
};

#endif