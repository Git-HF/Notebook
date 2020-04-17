#include "hf_socket.h"


namespace hf
{
Socket::Socket(int fd) : sockfd_(fd) 
{ 

}

Socket::~Socket()
{
    socketfuncs::CloseOrDie(sockfd_);
}

void Socket::SetReuse(bool is_on)
{
    int optval = is_on ? 1: 0;
    socketfuncs::SetReuseAddrOrDie(sockfd_, optval);
}
}