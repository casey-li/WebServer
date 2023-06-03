#include "src/server/webserver.h"


int main()
{
    /*
        端口，超时时间，ET模式，
        是否优雅关闭,连接池数量，线程数量，
        日志开关，日志等级，阻塞队列大小
    */
    WebServer web
    (9999, 10000, 3,
    true, 7, 3,
    true, 1, 1024);
    web.Start();
}