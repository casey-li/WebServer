#include "http_response.h"

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE{
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css "},
    {".js", "text/javascript "},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS{
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

void HttpResponse::Initialization(const std::string &resource_dir, const std::string &path, bool is_keep_alive = false, int code = -1)
{
    assert(resource_dir != "");
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
    ErrorHtml();
    AddStatusLine(buf);
    AddRespondHeader(buf);
    AddContent(buf);
}