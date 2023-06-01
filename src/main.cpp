#include "server/webserver.h"


int main()
{
    WebServer web(9999, 3, 5, 8);
    web.Start();
}