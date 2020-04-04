#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <cstdio>
#include <sys/wait.h>           //waitpid
#include <signal.h>             //sigaction
#include <errno.h>
#include <unistd.h>
#include <string>
#include "HttpRequest.h"
#include "HttpResponse.h"

#define BUFLEN 65536
#define DEFAULT_PORT 9876

// SIGCHLD信号处理函数
void sig_child(int signo)
{
    pid_t pid;
    int outStatus;
    // 这里必须使用非阻塞式，并且使用while，因为unix一般不对信号排队，当多个信号同时到达时，只会调用一个信号处理函数。所以在信号处理函数中应该循环判断是否有
    //子进程终止，否则会出现僵尸进程。
    while((pid = waitpid(-1, &outStatus, WNOHANG)) > 0)
    {
        //printf("%d child terminated\n", pid);
    }
}

//子进程处理新建连接
void child_process(int connectionFd)
{
    char buf[BUFLEN];
    std::string strbuf;
    int len;
    HttpRequest request;
    HttpResponse response;
    response.setDefaultResponse();
    while((len = read(connectionFd, buf, BUFLEN)) != 0)
    {
        if(len > 0)
        {
            strbuf.append(buf, buf+len);
            if(!request.parse(strbuf))
            {
                printf("http解析错误\n");
                request.reset();
                break;
            }

            if(request.is_got_all())
            {
                //使用移动语义，避免拷贝，加快速度
                string defaultResponse = std::move(response.returnResponse());
                request.reset();

                //向客户端返回响应。
                int nLeft = defaultResponse.size();
                const char* currentPos = defaultResponse.c_str();
                int nWriten;
                while(nLeft > 0)
                {
                    if((nWriten = write(connectionFd, currentPos, nLeft)) < 0)
                    {
                        //可能是被信号中断
                        if(errno == EINTR)
                            nWriten = 0;
                        else
                        {
                            printf("write error\n");
                            exit(0);
                        }
                    }

                    nLeft -= nWriten;
                    currentPos += nWriten;
                }
                
                //主动关闭连接,注意此处不能是close;
                shutdown(connectionFd, SHUT_WR);
            }
        }
        else
        {
            //可能被信号处理函数中断，重新判断
            if(errno == EINTR)
                continue;
            else
            {
                printf("read error\n");
                exit(0);
            }
        }
    }

    assert(close(connectionFd) == 0);
}

int main()
{
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenFd != -1);

    sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEFAULT_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt = 1;
    assert(setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0);

    assert(bind(listenFd, (sockaddr*)&addr, sizeof(addr)) == 0);

    assert(listen(listenFd, SOMAXCONN) == 0);

    //注册子进程终止时的信号处理函数，否则子进程会变成僵尸进程
    struct sigaction act, oact;
    // 这里写act.sa_handler，而不是act.__sigaction_handler
    act.sa_handler = sig_child;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    assert(sigaction(SIGCHLD, &act, &oact) == 0);

    int connectionFd;
    int pid;
    printf("------------------------------start listen----------------------------\n");
    while(1)
    {
        if((connectionFd = accept(listenFd, NULL, NULL)) < 0)
        {
            //从信号处理函数返回时，会中断慢系统调用，accept会返回错误，所以在这里需要检查一下。
            if(errno == EINTR)
                continue;
            else
            {
                printf("accept error\n");
                exit(0);
            }
        }

        if((pid = fork()) < 0)
        {
            printf("fork error\n");
            exit(0);
        }
        else if(pid == 0)           //子进程运行
        {
            /* code */
            assert(close(listenFd) == 0);        //子进程关闭监听套接字。
            child_process(connectionFd);
            exit(0);                //子进程结束。进程结束，其他套接字由系统自动关闭。
        }

        // --------父进程运行--------------
        //父进程关闭连接套接字。
        assert(close(connectionFd) == 0);
    }

    return 0;

}