#include "log.h"

#if 0
int main()
{
    int log_level = 1;
    int block_queue_size = 1024;
    Log::GetInstance()->Initialization(log_level, "./log", ".log", block_queue_size);
    // LOG_DEBUG("LOG_DEBUG");
    // LOG_INFO("LOG_INFO");
    // LOG_WARN("LOG_WARN");
    // LOG_ERROR("LOG_ERROR");

    std::string test1 = "test string";
    const char *test2 = "test char *";
    LOG_INFO("save info1 : %s", test1.c_str());
    LOG_INFO("save info2 : %s", test2);
}
#endif