#include "src/server/webserver.h"

int main()
{
    Json config;
    WebServer web (config.ParseConfig());
    web.Start();
}