#ifndef HF_SOCKET_H_
#define HF_SOCKET_H_

#include <netinet/in.h>

#include "hf_non_copy.h"
#include "hf_socket_funcs.h"


namespace hf
{

// 对socket描述符的封装，是一个RAII类型的类
class Socket : public NonCopy
{
    public:
        Socket(int fd);
        ~Socket();

        int Getfd() const { return sockfd_; }

        void BindAddress(const sockaddr_in * addr)
        {
            socketfuncs::BindOrDie(sockfd_, addr);
        }

        void Listen()
        {
            socketfuncs::ListenOrDie(sockfd_);
        }

        int Accept()
        {
            return socketfuncs::Accept(sockfd_);
        }

        // 设置套接字的SO_REUSEADDR标志
        // true标志开启SO_REUSEADDR，false表示关闭SO_REUSEADDR
        void SetReuse(bool is_on);


    private:
        const int sockfd_;

};
}
#endif