#include "HttpProcess.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include <cerrno>
#include <unistd.h>
#include <cstring>

// 读取客户端数据时缓冲区大小
#define BUFLEN 65536

void http_process(int connFd)
{
    char buf[BUFLEN];
    string strbuf;
    HttpRequest request;
    HttpResponse response;
    response.setDefaultResponse();
    int readCount;
    while((readCount = read(connFd, buf, BUFLEN)) != 0)
    {
        if(readCount > 0)
        {
            strbuf.append(buf, buf+readCount);
            if(!request.parse(strbuf))
            {
                printf("Http请求解析错误\n");
                request.reset();
                break;
            }

            if(request.is_got_all())            
            {
                string defaultResponse = std::move(response.returnResponse());
                request.reset();

                int nLeft = defaultResponse.size();
                const char* content = defaultResponse.c_str();
                int nWriten;

                while(nLeft > 0)
                {
                    if((nWriten = write(connFd, content, nLeft)) == -1)
                    {
                        printf("http response write error: %s\n", strerror(errno));
                        break; 
                    }

                    nLeft -= nWriten;
                    content += nWriten;
                } 
                break;
            }
        }
        else
        {
            printf("http read error: %s\n", strerror(errno));
            break;
        }
    }

    if(close(connFd) == -1)
    {
        printf("child close connFd error: %s\n", strerror(errno));
        exit(-1);
    }
}