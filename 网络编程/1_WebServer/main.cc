#include <sys/socket.h>     //SOMAXCONN
#include <cassert>
#include <cstdio>
#include <netinet/in.h>     //sockaddr_in头文件 htos等
#include <cstring>          //bzero()
#include <unistd.h>         //read/write
#include <errno.h>          //errno
#include "HttpResponse.h"
#include "HttpRequest.h"

#define BUFLEN 65536
#define DEFAULT_PORT 9876
int main()
{
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenFd >= 0);

    sockaddr_in addr;
    bzero(&addr, sizeof(addr)); 
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEFAULT_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //开启地址重用，为了当服务器关闭时，可以立马重启，如果不设置这个，可能存在TIME_WAIT状态的连接导致bind失败。
    int opt = 1;
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));     
    int flag = bind(listenFd, (sockaddr*)&addr, sizeof(addr));
    assert(flag == 0);

    flag = listen(listenFd, SOMAXCONN);
    assert(flag == 0);
    
    printf("--------------------------listening----------------------------\n");
    while(1)
    {
        int connectFd = accept(listenFd, NULL, NULL);

        assert(connectFd != -1);
        //printf("Connecting......\n");

        char buf[BUFLEN];
        int len;
        HttpRequest request;
        HttpResponse response;
        response.defaultResponse();
        string strbuf;
        
        while ((len = read(connectFd, buf, sizeof(buf))) != 0)
        {
            if(len > 0)
            {
                strbuf.append(std::begin(buf), buf+len);
                if(!request.parse(strbuf))
                {
                    request.reset();
                    printf("报文有错误");
                    shutdown(connectFd, SHUT_WR);
                }

                if(request.is_got_all())
                {
                    returnResponse(response, connectFd);
                    request.reset();
                    //printf("response over\n");
                    //注意此处不能是close，否则再次返回read会出错。
                    //使用webbench测试时，客户端会主动关闭连接。这里主要是为了浏览器访问。
                    shutdown(connectFd, SHUT_WR);
                }
            }
            else
            {
                printf("Error: %s", strerror(errno));
                return -1;
            }
        }

        close(connectFd);
        //printf("------------------------------------stop--------------------------------\n");
    }    
    return 0;
}