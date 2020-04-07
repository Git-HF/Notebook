#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include "HttpProcess.h"
#include "my_error.h"
#include <pthread.h>

//默认线程数量
#define DEFAULT_THREAD_NUM 8
//默认端口号
#define DEFAULTPORT 9876


pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


// 用于完成TCP服务端的开启过程，包括创建监听套接字socket、设置套接字REUSERADDR、绑定地址bind和监听listen。
// 函数返回监听套接字。
int tcp_server_init()
{
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd == -1)
    {
        printf("create listenFd error: %s\n", strerror(errno));
        exit(-1); 
    }

    // 可以设置为const类型。
    const int on = 1;
    // 设置套接字REUSEADDR模式。
    if(setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
    {
        printf("setsockopt REUSEADDR error: %s\n", strerror(errno));
        exit(-1);
    }

    sockaddr_in serverAddr;
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(DEFAULTPORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(listenFd, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
    {
        printf("bind serverAddr error: %s\n", strerror(errno));
        exit(1);
    }

    // 连接排队队列长度取最大值。
    if(listen(listenFd, SOMAXCONN) == -1)
    {
        printf("server listen error: %s\n", strerror(errno));
        exit(-1);
    }

    return listenFd;
}

//关闭文件描述符号，若出错，则直接退出进程
void Close(int fd)
{
    if(close(fd) == -1)
        my_error_quit("close fd error");
}

void* thread_main(void* args)
{
    // 设置线程分离状态
    pthread_detach(pthread_self());

    int listenFd = *((int*)args);

    int connFd;
    while(1)
    {   
        // 对accept上锁。
        pthread_mutex_lock(&lock);
        connFd = accept(listenFd, NULL, NULL);
        pthread_mutex_unlock(&lock);
        if(connFd == -1)
        {
            printf("accept error: %s", strerror(errno));
            continue;           //当accept出错时，继续监听不一定正确，所以这里可能需要更改
        }

        // connFd的关闭，在http_process中关闭的
        http_process(connFd);
    }
    return NULL;
}

int main()
{
    int listenFd = tcp_server_init();

    pthread_t tid;
    for(int i = 0; i < DEFAULT_THREAD_NUM; ++i)
    {
        //由于在这里，所有线程的listenFd是相同的，所有可以直接listenFd的指针。
        //如果listenFd变量是变化的，则不能这样传递。
        if(pthread_create(&tid, NULL, &thread_main, (void*)&listenFd) > 0)
        {
            my_error_quit("pthread create error\n");
        }
    }

    printf("-------------------------------start server--------------------\n");

    //主线程阻塞
    pause();

    return 0;
}