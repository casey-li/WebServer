#include "server/webserver.h"


int main()
{
    /*
        端口，超时时间，ET模式，是否优雅关闭
        连接池数量，线程数量
    */
    WebServer web
    (9999, 10000, 3,
    true, 7, 3);
    web.Start();
}