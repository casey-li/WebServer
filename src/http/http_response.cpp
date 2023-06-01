#include "http_response.h"

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE{
    {".html",   "text/html"},
    {".xml",    "text/xml"},
    {".xhtml",  "application/xhtml+xml"},
    {".txt",    "text/plain"},
    {".rtf",    "application/rtf"},
    {".pdf",    "application/pdf"},
    {".word",   "application/nsword"},
    {".png",    "image/png"},
    {".gif",    "image/gif"},
    {".jpg",    "image/jpeg"},
    {".jpeg",   "image/jpeg"},
    {".au",     "audio/basic"},
    {".mpeg",   "video/mpeg"},
    {".mpg",    "video/mpeg"},
    {".avi",    "video/x-msvideo"},
    {".gz",     "application/x-gzip"},
    {".tar",    "application/x-tar"},
    {".css",    "text/css"},
    {".js",     "text/javascript"},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_DESCRIPTION{
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_PATH{
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

HttpResponse::HttpResponse() : code_(-1), is_keep_alive_(false), path_(""),
                            resource_dir_(""), mmap_file_(nullptr)
{
    mmap_file_stat_ = {0};
}

HttpResponse::~HttpResponse()
{
    UnmapFile();
}

void HttpResponse::Initialization(const std::string &resource_dir, const std::string &path, bool is_keep_alive, int code)
{
    assert(resource_dir.size() > 0);
    if (mmap_file_) 
    {
        UnmapFile();
    }
    code_ = code;
    is_keep_alive_ = is_keep_alive;
    path_ = path;
    resource_dir_ = resource_dir;
    mmap_file_ = nullptr;
    mmap_file_stat_ = {0};
}

void HttpResponse::MakeResponse(Buffer &buf)
{
    // 获取请求文件的详细信息，判断是否为目录
    if (stat((resource_dir_ + path_).c_str(), &mmap_file_stat_) < 0 || S_ISDIR(mmap_file_stat_.st_mode))
    {
        code_ = 404;
    }
    // 判断访问权限, S_IROTH, S_IWOTH, S_IXOTH 分别表示其他人能否读, 写, 执行
    else if (!(mmap_file_stat_.st_mode & S_IROTH))
    {
        code_ = 403;
    }
    else if (code_ == -1)
    {
        code_ = 200;
    }
    FindFile();
    AddStatusLine(buf);
    AddRespondHeader(buf);
    AddContent(buf);
}

void HttpResponse::FindFile()
{
    if (CODE_PATH.count(code_))
    {
        path_ = CODE_PATH.at(code_);
        stat((resource_dir_ + path_).c_str(), &mmap_file_stat_);
    }
}

void HttpResponse::AddStatusLine(Buffer &buf)
{
    code_ = CODE_DESCRIPTION.count(code_) ? code_ : 400;
    std::string description = CODE_DESCRIPTION.at(code_);
    buf.Append("HTTP/1.1 " + std::to_string(code_) + " " + description + "\r\n");
}

void HttpResponse::AddRespondHeader(Buffer &buf)
{
    buf.Append("Connection: ");
    if (is_keep_alive_)
    {
        buf.Append("keep-alive\r\n");
        buf.Append("keep-alive: max=6, timeout=120\r\n");
    }
    else
    {
        buf.Append("close\r\n");
    }
    buf.Append("Content-type: " + GetFileType() + "\r\n");
}

void HttpResponse::AddContent(Buffer &buf)
{
    int fd = open((resource_dir_ + path_).c_str(), O_RDONLY);
    if (fd < 0)
    {
        ErrorContent(buf, "FileNotFound!");
        return;
    }
    // 打印日志
    // 将文件映射到内存，提高访问速度
    void *ptr = mmap(nullptr, GetFileSize(), PROT_READ, MAP_PRIVATE, fd, 0);
    if (ptr == MAP_FAILED)
    {
        ErrorContent(buf, "FileNotFound");
        return;
    }
    mmap_file_ = static_cast<char *>(ptr);
    close(fd);
    buf.Append("Content-length: " + std::to_string(GetFileSize()) + "\r\n\r\n");
}

void HttpResponse::UnmapFile()
{
    if (mmap_file_)
    {
        munmap(mmap_file_, GetFileSize());
        mmap_file_ = nullptr;
    }
}

std::string HttpResponse::GetFileType()
{
    // 判断文件类型
    std::string::size_type idx = path_.find_last_of('.');
    if (idx != std::string::npos)
    {
        std::string suffix = path_.substr(idx);
        if (SUFFIX_TYPE.count(suffix))
        {
            return SUFFIX_TYPE.at(suffix);
        }
    }
    return "text/plain";
}

void HttpResponse::ErrorContent(Buffer &buf, const std::string &message)
{
    std::string body = "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    std::string description = CODE_DESCRIPTION.count(code_) ? CODE_DESCRIPTION.at(code_) : "Bad Request";
    body += std::to_string(code_) + " : " + description  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>WebServer</em></body></html>";    
    buf.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buf.Append(body);
}