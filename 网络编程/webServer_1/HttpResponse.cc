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

void HttpResponse::defaultResponse()
{
    setVersion("HTTP/1.1");
    setState("200");
    setStateDescription("OK");
    setHead("Content-Type", "text/html");
    setBody("./HelloWorld.html");
}


void returnResponse(const HttpResponse& response, int connectFd)
{
    const char* crlf = "\r\n";
    string tmp;

    tmp = response.getVersion() + " ";
    //write(connectFd, tmp.c_str(), tmp.size());

    tmp += response.getState() + " ";
    //write(connectFd, tmp.c_str(), tmp.size());

    tmp += response.getStateDescription() + crlf;
    //write(connectFd, tmp.c_str(), tmp.size());

    for(const auto & item : response.getHeads())
    {
        tmp += item.first + ":" + item.second + crlf;
        //write(connectFd, tmp.c_str(), tmp.size());
    }

    tmp += crlf;
    //write(connectFd, tmp.c_str(), tmp.size());

    tmp += response.getBody();
    write(connectFd, tmp.c_str(), tmp.size());

}