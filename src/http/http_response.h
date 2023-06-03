#ifndef HTTP_RESPONSE_H
#define HTTP_RESPENSE_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse
{
public:
    HttpResponse();

    ~HttpResponse();

    void Initialization(const std::string &resource_dir, const std::string &path, bool is_keep_alive = false, int code = -1);

    // 生成响应报文
    void MakeResponse(Buffer &buf);

    // 返回请求文件的内存映射得到的地址
    char *GetFileAddr()
    {
        return mmap_file_;
    }

    void UnmapFile();

    
    size_t GetFileSize() const
    {
        return mmap_file_stat_.st_size;
    }

    // 生成错误信息的网页
    void ErrorContent(Buffer &buf, const std::string &message);

    int GetCode() const
    {
        return code_;
    }


private:
    void AddStatusLine(Buffer &buf);

    void AddRespondHeader(Buffer &buf);

    void AddContent(Buffer &buf);

    // 根据路径找到文件并获取文件状态信息
    void FindFile();

    // 根据请求文件的后缀返回文件类型
    std::string GetFileType();

    int code_;

    bool is_keep_alive_;

    std::string path_, resource_dir_;

    char *mmap_file_; // 保存请求文件内存映射的地址

    struct stat mmap_file_stat_;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;  // 保存请求文件对应的后缀描述信息

    static const std::unordered_map<int, std::string> CODE_DESCRIPTION;     // 保存状态码对应的描述

    static const std::unordered_map<int, std::string> CODE_PATH;            // 保存状态码对应的文件

};

#endif