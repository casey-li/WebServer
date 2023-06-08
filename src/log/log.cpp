#include "log.h"

#include "log.h"

Log::Log()
{
    path_ = suffix_ = "";
    is_open_ = true;
    is_async_ = false;
    line_count_ = today_ = 0;
    fp_ = nullptr;
    deque_ = nullptr;
    write_thread_ = nullptr;
}

Log::~Log() 
{
    if (write_thread_ && write_thread_->joinable()) 
    {
        while(!deque_->Empty()) 
        {
            deque_->Flush();
        }
        deque_->Close();
        write_thread_->join();
    }
    if(fp_) 
    {
        std::lock_guard<std::mutex> locker(mtx_);
        Flush();
        fclose(fp_);
    }
}

void Log::Initialization(int level, std::string path,
        std::string suffix, int max_queue_capacity)
{
    is_open_ = true;
    is_async_ = false;
    level_ = level;
    if(max_queue_capacity > 0) 
    {
        is_async_ = true;
        if(!deque_) 
        {
            deque_ = std::make_unique<BlockDeque<std::string>>(max_queue_capacity);
            write_thread_ = std::make_unique<std::thread>(&Log::ThreadFlushLog);
        }
    } 
    line_count_ = 0;
    // 获取时间并初始化日志文件名字，年_月_日.log
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    std::tm t = *std::localtime(&now_time);
    today_ = t.tm_mday;

    path_ = std::move(path);
    suffix_ = std::move(suffix);
    char fileName[log_name_len] = {0};
    snprintf(fileName, log_name_len - 1, "%s/%04d_%02d_%02d%s", 
        path_.c_str(), t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_.c_str());

    //printf("path : %s", fileName);


    {
        std::lock_guard<std::mutex> locker(mtx_);
        buf_.RetrieveAll();
        if(fp_) 
        { 
            Flush();
            fclose(fp_); 
        }
        fp_ = fopen(fileName, "a");
        if(fp_ == nullptr) 
        {
            mkdir(path_.c_str(), 0777);
            fp_ = fopen(fileName, "a");
        } 
        assert(fp_ != nullptr);
    }
}

void Log::Write(int level, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;
    // 检查日期是否正确，是否超行，超行的话新建文件 (-n)   
    if (today_ != t.tm_mday || (line_count_ && (line_count_ % MAX_LINES == 0)))
    {
        std::unique_lock<std::mutex> locker(mtx_);
        locker.unlock();
        char newFile[log_name_len];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        if (today_ != t.tm_mday)
        {
            snprintf(newFile, log_name_len - 72, "%s/%s%s", path_.c_str(), tail, suffix_.c_str());
            today_ = t.tm_mday;
            line_count_ = 0;
        }
        else 
        {
            snprintf(newFile, log_name_len - 72, "%s/%s-%d%s", path_.c_str(), tail, (line_count_ / MAX_LINES), suffix_.c_str());
        }
        locker.lock();
        Flush();
        fclose(fp_);
        fp_ = fopen(newFile, "a");
        assert(fp_ != nullptr);
    }
    {
        std::unique_lock<std::mutex> locker(mtx_);
        line_count_++;
        int n = snprintf(buf_.GetWritePtr(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
                    
        buf_.UpdateWritePos(n);
        AppendLogLevelTitle(level);
        va_start(vaList, format);
        int m = vsnprintf(buf_.GetWritePtr(), buf_.WritableBytes(), format, vaList);
        va_end(vaList);

        buf_.UpdateWritePos(m);
        buf_.Append("\n\0", 2);

        if(is_async_ && deque_ && !deque_->Full()) 
        {
            deque_->PushBack(buf_.RetrieveAllToStr());
        } 
        else 
        {
            fputs(buf_.GetReadPtr(), fp_);
        }
        buf_.RetrieveAll();
    }    
}

void Log::AppendLogLevelTitle(int level) 
{
    switch(level) 
    {
    case 0:
        buf_.Append("[debug]: ", 9);
        break;
    case 1:
        buf_.Append("[info] : ", 9);
        break;
    case 2:
        buf_.Append("[warn] : ", 9);
        break;
    case 3:
        buf_.Append("[error]: ", 9);
        break;
    default:
        buf_.Append("[info] : ", 9);
        break;
    }
}

void Log::Flush() 
{
    if(is_async_) 
    { 
        deque_->Flush(); 
    }
    fflush(fp_);
}

void Log::AsyncWrite() 
{
    std::string str = "";
    while(deque_->Pop(str)) 
    {
        std::lock_guard<std::mutex> locker(mtx_);
        fputs(str.c_str(), fp_);
    }
}
