#include "HttpResponse.h"
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

HttpResponse::HttpResponse(/* args */)
{
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::setBody(const char* file_name)
{
    int fd = open(file_name, O_RDONLY);
    assert(fd != -1);

    char buf[65536];
    int len;
    while((len = read(fd, buf, sizeof(buf))) > 0)
    {
        m_body.append(buf, buf + len);
    }

    if(len != 0)
    {
        printf("read body error\n");
        exit(-1);
    }

    close(fd);
}

void HttpResponse::setDefaultResponse()
{
    setVersion("HTTP/1.1");
    setState("200");
    setStateDescription("OK");
    setHead("Content-Type", "text/html");
    setBody("./HelloWorld.html");
}


string HttpResponse::returnResponse()
{
    const char* crlf = "\r\n";
    string tmp;

    //版本号
    tmp = getVersion() + " ";
    //状态码
    tmp += getState() + " ";
    //状态描述
    tmp += getStateDescription() + crlf;
    //响应头
    for(const auto & item : getHeads())
    {
        tmp += item.first + ":" + item.second + crlf;
    }
    //空行
    tmp += crlf;
    //响应体
    tmp += getBody();
    return tmp;
}