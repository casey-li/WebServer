#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <regex>
#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../mysql_connection_pool/mysql_connection_pool.h"

class HttpRequest
{
public:
    // 解析客户端请求时，主状态机的状态
    enum PARSE_STATUS
    {
        PARSE_REQUEST_LINE = 0, // 当前正在分析请求行
        PARSE_HEADER,           // 当前正在分析头部字段
        PARSE_CONTENT,          // 当前正在解析请求体
        FINISH,                 // 解析完成
    };

    // // 服务器处理HTTP请求的可能结果，报文解析的结果
    // enum HTTP_STATUS
    // {
    //     ON_REQUEST = 0,    // 请求不完整，需要继续读取客户数据
    //     GET_REQUEST,       // 表示获得了一个完成的客户请求
    //     SYNTAX_ERROR,      // 表示客户请求语法错误
    //     NO_RESOURCE,       // 表示服务器没有资源
    //     FORBIDDEN_ACCESS,  // 表示客户对资源没有足够的访问权限
    //     GET_FILE,          // 文件请求,获取文件成功
    //     INTERNAL_ERROR,    // 表示服务器内部错误
    //     CLOSED_CONNECTION, // 表示客户端已经关闭连接了
    // };

    HttpRequest()
    {
        Initialization();
    }

    ~HttpRequest() = default;

    void Initialization();

    // 采用有限状态机模型解析请求行，请求头部，请求数据
    bool Parse(Buffer &buf);

    std::string GetPath() const
    {
        return path_;
    }

    std::string &GetPath()
    {
        return path_;
    }

    std::string GetMethod()
    {
        return method_;
    }

    std::string GetVersion()
    {
        return version_;
    }

    // 根据解析 POST 请求体的结果返回 value
    std::string GetPost(const std::string &key) const;

    // 根据解析 POST 请求体的结果返回 value
    std::string GetPost(const char *key) const;

    // 根据解析得到的请求头部中的信息返回是否持久连接
    bool GetIsKeepAlive() const;

private:
    // 解析 Http 请求行内容，包括 方法，URL 和 HTTP版本，它们以空格划分}
    bool ParseRequestLine(const std::string &line);

    // 解析请求头部信息，请求头部中每行都以 : 划分字段名和字段值
    void ParseHeader(const std::string &line);

    // 保存请求数据，调用 ParsePost() 解析
    void ParseContent(const std::string &line);

    // 给请求的 path_ 拼接 .html，默认为 /index.html
    void ParsePath();

    // 解析 POST 请求，修改 path_ 为登录或注册或错误页面
    void ParsePost();

    // 解析以 URL 编码形式提交的表单数据，保存解析得到的键值对结果
    void ParseFromUrlEncoded();

    // 解析 URL 编码的特殊字符
    static int ConverHex(char ch);

    // 用户请求注册信息，连接数据库添加用户的账号，密码
    bool UserRegister(const std::string& name, const std::string& pwd);

    // 连接数据库验证用户的登录信息
    bool UserVerify(const std::string& name, const std::string& pwd);

    PARSE_STATUS parse_status_; // 当前解析请求报文的状态

    std::string method_, path_, version_, body_;                // 保存解析得到的结果

    std::unordered_map<std::string, std::string> header_;       // 保存请求头部中字段名和字段值的映射

    std::unordered_map<std::string, std::string> post_;         //保存解析 POST 请求体得到的键值对

    static const std::unordered_set<std::string> DEFAULT_HTML;  // 保存所有网页的名字，用于检验请求的网页是否存在

    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG; // 方便检查是登录还是注册的 POST 请求
};

#endif