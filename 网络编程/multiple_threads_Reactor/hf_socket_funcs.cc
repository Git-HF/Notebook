#include "hf_socket_funcs.h"

#include <unistd.h>

#include <cerrno>

namespace hf
{

void socketfuncs::CloseOrDie(int sockfd)
{
    if(::close(sockfd) < 0)
    {
        ErrorAndQuit("[socketfuncs::CloseOrDie] error");
    }
}

void socketfuncs::BindOrDie(int sockfd, const sockaddr_in * addr)
{
    // 注意最后一个参数不能写成sizeof(addr),最后一个参数是一个指针
    if(::bind(sockfd, (sockaddr*)(addr), sizeof(sockaddr_in)) < 0)
    {
        ErrorAndQuit("[socketfuncs::BindOrDie] error");
    }
}

void socketfuncs::ListenOrDie(int sockfd)
{
    if(::listen(sockfd, SOMAXCONN) < 0)
    {
        ErrorAndQuit("[socketfuncs::ListenOrDie] error");
    }
}

int socketfuncs::Accept(int sockfd)
{
    //直接将连接套接字置为非阻塞
    int res = ::accept4(sockfd, NULL, NULL, SOCK_CLOEXEC | SOCK_NONBLOCK);

    if(res < 0)
    {
        switch (errno)
        {
            // 一般错误
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO: // ???
            case EPERM:
            case EMFILE:    // 文件描述符耗尽，上层程序应该单独处理
                OutputError("[socketfuncs::Accept] error");
                break;

            // 致命错误
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                ErrorAndQuit("[socketfuncs::Accept] error");
                break;

            // 未知的错误类型，也直接退出
            default:
                ErrorAndQuit("[socketfuncs::Accept] unknow error");
                break;
        }
    }

    return res;
}

int socketfuncs::CreateNonBlockSocketOrDie()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(sockfd < 0)
    {
        ErrorAndQuit("socketfuncs::CreateNonBlockSocketOrDie error");
    }
    
    return sockfd;
}

void socketfuncs::SetReuseAddrOrDie(int fd, int optval)
{
    if(::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        ErrorAndQuit("socketfuncs::SetReuseAddrOtDie");
    }
}

int socketfuncs::GetSocketError(int sockfd)
{
    int optval;
    socklen_t optlen = sizeof(optval);

    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        return errno;
    }
    else
    {
        return optval;
    }
}

void socketfuncs::ShutDownWrite(int sockfd)
{
    if(::shutdown(sockfd, SHUT_WR) < 0)
    {
        // ENOTCONN表示对端已经关闭了连接，所以达到一样的效果
        if(errno != ENOTCONN)
            ErrorAndQuit("sockectfuncs::ShutDownWrite error");
    }
}

}