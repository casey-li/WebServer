#include "http_request.h"

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
    "/index",
    "/register",
    "/login",
    "/welcome",
    "/video",
    "/picture",
};

const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG{
    {"/register.html",  0},
    {"/login.html",     1},
};

void HttpRequest::Initialization()
{
    parse_status_ = PARSE_REQUEST_LINE;
    method_ = path_ = version_ = body_ = "";
    header_.clear();
    post_.clear();
}

bool HttpRequest::Parse(Buffer &buf)
{
    const std::vector<char> end_flag{'\r', '\n'};
    if (buf.ReadableBytes() <= 0)
    {
        return false;
    }
    while (buf.ReadableBytes() && parse_status_ != FINISH)
    {
        // 调用 search 找第一次出现 "\r\n" 的位置传入 在哪找 以及 查找结果 的范围
        char *line_end = std::search(buf.GetReadPtr(), buf.GetWritePtr(), end_flag.begin(), end_flag.end());
        std::string line(buf.GetReadPtr(), line_end);
        switch (parse_status_)
        {
            case PARSE_REQUEST_LINE:
            {
                if (!ParseRequestLine(line))
                {
                    return false;
                }
                ParsePath();
                break;
            }
            case PARSE_HEADER:
            {
                ParseHeader(line);
                if (buf.ReadableBytes() <= 2)
                {
                    // 等于2的话表明只剩下一个空行了，没有请求数据
                    parse_status_ = FINISH;
                }
                break;
            }
            case PARSE_CONTENT:
            {
                ParseContent(line);
                break;
            }
            default:
                break;
        }
        if (line_end == buf.GetWritePtr())
        {
            break; // 所有数据都读完了
        }
        buf.RetrieveUntil(line_end + 2);
    }
    LOG_DEBUG("Request method: %s, path: %s, version: %s", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

bool HttpRequest::ParseRequestLine(const std::string &line)
{
    // ^ 和 $ 分别表示字符串的开头结尾，([^ ]*) 表示匹配任意长度的非空格字符串
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch sub_match;
    // 匹配成功的话，匹配结果保存在 sub_match 中
    if (std::regex_match(line, sub_match, patten))
    {
        method_ = sub_match[1];
        path_ = sub_match[2];
        version_ = sub_match[3];
        parse_status_ = PARSE_HEADER;
        return true;
    }
    LOG_ERROR("ParseRequestLine Error!");
    return false;
}

void HttpRequest::ParsePath()
{
    if (path_ == "/")
    {
        path_ = "/index.html";
    }
    else
    {
        for (auto &s : DEFAULT_HTML)
        {
            if (path_ == s)
            {
                path_ += ".html";
                break;
            }
        }
    }
}

void HttpRequest::ParseHeader(const std::string &line)
{
    // 匹配以 : 为分隔符的请求头部中的字段名和字段值
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch sub_match;
    if (std::regex_match(line, sub_match, patten))
    {
        header_[sub_match[1]] = sub_match[2];
    }
    else // 不匹配说明到最后一行空行了，转换状态
    {
        parse_status_ = PARSE_CONTENT;
    }
}

void HttpRequest::ParseContent(const std::string &line)
{
    body_ = line;
    ParsePost();
    parse_status_ = FINISH;
    LOG_DEBUG("ParseContent: %s, len: %d", line.c_str(), line.size());
}

void HttpRequest::ParsePost()
{
    // 检查请求方法是否为POST，以及请求头部的 Content-Type 是否为 application/x-www-form-urlencoded
    // 如果条件满足，表示接收到的是以URL编码形式提交的表单数据，调用 ParseFromUrlEncoded 解析
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded")
    {
        ParseFromUrlEncoded();
        if (DEFAULT_HTML_TAG.count(path_))
        {
            int act = DEFAULT_HTML_TAG.at(path_);
            // 根据登录或者注册的结果来决定返回哪个页面
            if (act == 0) // 用户请求注册
            {
                LOG_INFO("user: %s requires sign up", post_["username"].c_str());
                path_ = UserRegister(post_["username"], post_["password"]) ? "/welcome.html" : "/error.html";
            }
            else if (act == 1) // 用户请求登录
            {
                LOG_INFO("user: %s requires log in", post_["username"].c_str());
                path_ = UserVerify(post_["username"], post_["password"]) ? "/welcome.html" : "/error.html";
            }
        }
    }
}

void HttpRequest::ParseFromUrlEncoded()
{
    size_t len = body_.size();
    if (len == 0)
    {
        return ;
    }
    std::string key, value;
    size_t num = 0, cur = 0, last = 0;
    for (; cur < len; ++cur) // 遍历每个字符
    {
        switch (body_[cur]) 
        {
            case '=': // '=' 表示找到了键的结束位置，更新 last 的值为下一个键值对的起始位置
            {
                key = body_.substr(last, cur - last);
                last = cur + 1;
                break;
            }
            case '+': // '+'，表示遇到了 URL 编码的空格，将其替换为实际空格字符 ' '
            {
                body_[cur] = ' ';
                break;
            }
            case '%': // '%'，表示遇到了 URL 编码的特殊字符，需要将其转换为对应的字符
            {
                num = ConverHex(body_[cur + 1]) * 16 + ConverHex(body_[cur + 2]);
                body_[cur + 2] = num % 10 + '0';
                body_[cur + 1] = num / 10 + '0';
                cur += 2;
                break;
            }
            case '&': // '&'，则表示找到了一个完整的键值对，将键值对存储到 post_ 哈希表中
            {
                value = body_.substr(last, cur - last);
                last = cur + 1;
                post_[key] = value;
                break;
            }
            default:
                break;
        }
    }
    assert(last <= cur);
    // 保存最后一个键值对
    if (post_.count(key) == 0 && last < cur)
    {
        value = body_.substr(last, cur - last);
        post_[key] = value;
    }
}

int HttpRequest::ConverHex(char ch)
{
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return ch;
}

bool HttpRequest::UserRegister(const std::string& name, const std::string& pwd)
{
    if (name.size() == 0 || pwd.size() == 0)
    {
        return false;
    }
    LOG_INFO("Register name: %s", name.c_str());
    std::string sql = "SELECT count(*) from user where username = " + name;
    std::shared_ptr<MysqlConnection> ptr = MysqlConnectionPool::GetInstance()->GetConnection();
    if (!ptr->Query(sql) || !ptr->Next())
    {
        LOG_INFO("Failed to read database information!");
        return false;
    }
    // 用户名重复，注册失败
    if (ptr->GetValue(0) == "1") 
    {
        LOG_INFO("User name: %s has been used!", name.c_str());
        return false;
    }
    sql = "INSERT INTO user VALUES('" + name + "', '" + pwd +"')";
    if (!ptr->Update(sql))
    {
        LOG_ERROR("Insert new user error!");
        return false;
    }
    return true;
}

bool HttpRequest::UserVerify(const std::string& name, const std::string& pwd)
{
    if (name.size() == 0 || pwd.size() == 0)
    {
        return false;
    }
    LOG_INFO("Verify user name: %s, passward: %s", name.c_str(), pwd.c_str());
    std::string sql = "SELECT username, password from user where username = '" + name + "'";
    std::shared_ptr<MysqlConnection> ptr = MysqlConnectionPool::GetInstance()->GetConnection();
    // 查询
    if (!ptr->Query(sql))
    {
        LOG_INFO("Query failed!");
        return false;
    }
    // 验证结果
    while (ptr->Next())
    {
        if (ptr->GetValue(1) == pwd)
        {
            return true;
        }
    }
    LOG_INFO("User input error password: %s", pwd.c_str());
    return false;
}

std::string HttpRequest::GetPost(const std::string &key) const
{
    assert(key.size() > 0);
    if (post_.count(key))
    {
        return post_.at(key);
    }
    return "";
}

std::string HttpRequest::GetPost(const char *key) const
{
    assert(key);
    if (post_.count(key))
    {
        return post_.at(key);
    }
    return "";
}

bool HttpRequest::GetIsKeepAlive() const
{
    if (header_.count("Connection"))
    {
        return header_.at("Connection") == "keep-alive" && version_ == "1.1"; 
    }
    return false;
}