#include <sys/socket.h>     //SOMAXCONN
#include <cassert>
#include <cstdio>
#include <netinet/in.h>     //sockaddr_in头文件 htos等
#include <cstring>          //bzero()
#include <unistd.h>         //read/write
#include <errno.h>          //errno
#include "HttpResponse.h"

#define BUFLEN 65536
int main()
{
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenFd >= 0);

    sockaddr_in addr;
    bzero(&addr, sizeof(addr)); 
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int flag = bind(listenFd, (sockaddr*)&addr, sizeof(addr));
    assert(flag == 0);

    flag = listen(listenFd, SOMAXCONN);
    assert(flag == 0);
    
    //while(1)
    {
        int connectFd = accept(listenFd, NULL, NULL);

        printf("Connecting......\n");

        char buf[BUFLEN];
        int len;
        while ((len = read(connectFd, buf, sizeof(buf))) != 0)
        {
            if(len > 0)
            {
                write(connectFd, buf, len);
                write(STDOUT_FILENO, buf, len);
                HttpResponse response;
                response.defaultResponse();
                returnResponse(response, connectFd);

                printf("response over\n");
                //close(connectFd);
                //shutdown(connectFd, SHUT_WR);
            }
            else
            {
                printf("Error: %s", strerror(errno));
                return -1;
            }
        }

        close(connectFd);
        printf("stop......\n");
    }    
    return 0;
}