#include "http_request.h"

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
    "/index",
    "/register",
    "/login",
    "/welcome",
    "/video",
    "/picture",
};

const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG{
    {"/register.html",  0},
    {"/login.html",     1},
};

void HttpRequest::Initialization()
{
    parse_status_ = PARSE_REQUEST_LINE;
    method_ = path_ = version_ = body_ = "";
    header_.clear();
    post_.clear();
}