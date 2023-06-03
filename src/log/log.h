#ifndef LOG_H
#define LOG_H

#include <string>
#include <thread>
#include <mutex>
#include <sys/stat.h>   // mkdir
#include <stdarg.h>     // vastart va_end
#include <sys/time.h>
#include <functional>
#include <iostream>
#include "../buffer/buffer.h"
#include "block_deque.hpp"

/*
    日志级别
    0: debug
    1: info
    2: warn
    3: error
*/
class Log
{
public:
    //  阻塞队列容量大于 0 表示异步，否则同步
    void Initialization(int level = 1, std::string path = "./log",
        std::string suffix = "./log", int max_queue_capacity = 1024);

    static Log *GetInstance()
    {
        static Log log;
        return &log;
    }

    // 异步写日志，调用 AsyncWrite()
    static void ThreadFlushLog()
    {
        Log::GetInstance()->AsyncWrite();
    }

    // 将输出内容按照标准格式整理
    void Write(int level, const char *format, ...);

    // 刷新到缓冲区
    void Flush();

    int GetLevel()
    {
        std::lock_guard<std::mutex> locker(mtx_);
        return level_;
    }

    void SetLevel(int level)
    {
        std::lock_guard<std::mutex> locker(mtx_);
        level_ = level;     
    }

    bool IsOpen()
    {
        return is_open_;
    }

private:
    Log();

    virtual ~Log();

    // 添加日志级别信息
    void AppendLogLevelTitle(int level);

    void AsyncWrite();

private:
    static const int log_path_len = 256;

    static const int log_name_len = 256;

    static const int MAX_LINES = 50000;

    std::string path_, suffix_;

    bool is_open_, is_async_;

    int level_;

    int max_lines_;

    int line_count_;

    int today_;

    Buffer buf_;

    FILE *fp_;

    std::unique_ptr<BlockDeque<std::string>> deque_;

    std::unique_ptr<std::thread> write_thread_;

    std::mutex mtx_;
};

// ##__VA_ARGS__ 表示将可变参数列表展开，并与前面的参数组合成一个完整的函数调用

#define LOG_BASE(level, format, ...) \
    Log* log = Log::GetInstance();\
    if (log->IsOpen() && log->GetLevel() <= level) {\
        log->Write(level, format, ##__VA_ARGS__); \
        log->Flush();\
    };

#define LOG_DEBUG(format, ...) { LOG_BASE(0, format, ##__VA_ARGS__) };

#define LOG_INFO(format, ...) { LOG_BASE(1, format, ##__VA_ARGS__) };

#define LOG_WARN(format, ...) { LOG_BASE(2, format, ##__VA_ARGS__) };

#define LOG_ERROR(format, ...) { LOG_BASE(3, format, ##__VA_ARGS__) };

#endif