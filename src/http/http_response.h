#ifndef HTTP_RESPONSE_H
#define HTTP_RESPENSE_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "../buffer/buffer.h"

class HttpResponse
{
public:
    HttpResponse();

    ~HttpResponse();

    void Initialization(const std::string &resource_dir, const std::string &path, bool is_keep_alive = false, int code = -1);

    void MakeResponse(Buffer &buf);

    char * File();

    size_t GetFileSize();

    void UnmapFile();

private:
    void AddStatusLine(Buffer &buf);

    void AddRespondHeader(Buffer &buf);

    void AddContent(Buffer &buf);

    void ErrorHtml();

    int code_;

    bool is_keep_alive_;

    std::string path_;

    std::string resource_dir_;

    char *mmap_file_;

    struct stat mmap_file_stat_;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;

    static const std::unordered_map<int, std::string> CODE_STATUS;

    static const std::unordered_map<int, std::string> CODE_PATH;

};

#endif