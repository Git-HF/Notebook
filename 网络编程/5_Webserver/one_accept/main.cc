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
#include <queue>

//默认线程数量
#define DEFAULT_THREAD_NUM 8
//默认端口号
#define DEFAULTPORT 9876


static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

using std::queue;
queue<int> socketFdBuf;           //用来存储连接套接字的缓冲区，主线程向其中添加文件描述符，子线程在其中取文件描述符

// 用于完成TCP服务端的开启过程，包括创建监听套接字socket、设置套接字REUSERADDR、绑定地址bind和监听listen。
// 函数返回监听套接字。
int tcp_server_init()
{
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd == -1)
        my_error_quit("create listenFd error");

    // 可以设置为const类型。
    const int on = 1;
    // 设置套接字REUSEADDR模式。
    if(setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
        my_error_quit("setsockopt REUSEADDR error");

    sockaddr_in serverAddr;
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(DEFAULTPORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(listenFd, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
        my_error_quit("bind serverAddr error");

    // 连接排队队列长度取最大值。
    if(listen(listenFd, SOMAXCONN) == -1)
        my_error_quit("server listen error");

    return listenFd;
}

//关闭文件描述符号，若出错，则直接退出进程
void Close(int fd)
{
    if(close(fd) == -1)
        my_error_quit("close fd error");
}

void* thread_main(void*)
{
    // 设置线程分离状态
    pthread_detach(pthread_self());

    int connFd;
    while(1)
    {
        pthread_mutex_lock(&lock);
        // 注意这里不能是if
        while(socketFdBuf.size() == 0)
            pthread_cond_wait(&condition, &lock);
        connFd = socketFdBuf.front();
        socketFdBuf.pop();
        pthread_mutex_unlock(&lock);
        
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
        if(pthread_create(&tid, NULL, &thread_main, NULL) > 0)
        {
            my_error_quit("pthread create error\n");
        }
    }

    printf("-------------------------------start server--------------------\n");

    int connFd;
    while(1)
    {
        connFd = accept(listenFd, NULL, NULL);
        if(connFd == -1)
            my_error_quit("accept error");
        
        pthread_mutex_lock(&lock);
        socketFdBuf.push(connFd);
        pthread_cond_signal(&condition);
        pthread_mutex_unlock(&lock);
    }
    return 0;
}