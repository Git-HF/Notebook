#include <assert.h>
#include <pthread.h>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <string>
#include "HttpRequest.h"
#include "HttpResponse.h"


#define BUFLEN 65536
#define DEFAULTPORT 9876

// 阻塞IO+thread架构的服务器不涉及信号，因此不需要考虑处理信号中断慢速系统调用

void* thread_process(void* arg)
{
    //设置为分离状态
    assert(pthread_detach(pthread_self()) == 0);
    int* p_connectFd = (int*)arg;

    char buf[BUFLEN];
    string strbuf;
    HttpRequest request;
    HttpResponse response;
    response.setDefaultResponse();
    int readCount;
    while((readCount = read(*p_connectFd, buf, BUFLEN)) != 0)
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
                    if((nWriten = write(*p_connectFd, content, nLeft)) == -1)
                    {
                        printf("thread_process write error: %s", strerror(errno));
                        assert(close(*p_connectFd) == 0);
                        delete p_connectFd;
                        //如果在这里break，则无法跳出外层循环
                        pthread_exit(NULL);
                    }

                    nLeft -= nWriten;
                    content += nWriten;
                } 
                break;

                //如果这里使用shutdown，在使用webbench测试时，最后总会显示"shutdown error Transport endpoint is not connected"错误。
                //不知道为什么？？？
                /*
                if(shutdown(*p_connectFd, SHUT_WR) == -1)
                {
                    printf("shutdown error %s\n", strerror(errno));
                    break;
                }
                */
            }
        }
        else
        {
            printf("thread_process read error: %s", strerror(errno));
            break;
        }
    }

    //进程结束时，应该主动关闭套接字，并且释放动态内存。
    assert(close(*p_connectFd) == 0);
    delete p_connectFd;
    return NULL;
}

int main()
{
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenFd != -1);

    sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEFAULTPORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt = 1;
    assert(setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0);

    assert(bind(listenFd, (sockaddr*)&addr, sizeof(addr)) == 0);

    assert(listen(listenFd, SOMAXCONN) == 0);
    printf("--------------------------start listen-------------------\n");

    int connectFd;
    pthread_t tid;
    int* p_connectFd = NULL;
    while(1)
    {
        if((connectFd = accept(listenFd, NULL, NULL)) == -1)
        {
            printf("accept error: %s\n", strerror(errno));
            continue;
        }

        p_connectFd = new int(connectFd);
        assert(p_connectFd != NULL);

        //pthread系列函数在执行成功时返回0，否则返回正值，与其他一般返回-1的情况不太一样
        //传递给线程启动函数的连接套接字使用指向堆的指针（详细可参考UNP第26章）
        if(pthread_create(&tid, NULL, &thread_process, (void*)p_connectFd) != 0)
        {
            printf("pthread_create error\n");
            //如果创建线程失败，注意释放。
            delete p_connectFd;
            assert(close(connectFd) == 0);
        }

        // 线程创建成功时，不能关闭连接套接字，因为线程之间共享文件描述符。
        p_connectFd = NULL;
    }

    return 0;
}
