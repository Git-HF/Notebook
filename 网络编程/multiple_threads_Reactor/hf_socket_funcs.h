#ifndef HF_SOCKET_FUNCS_H_
#define HF_SOCKET_FUNCS_H_

#include <netinet/in.h>

#include "hf_utils.h"

namespace hf
{

namespace socketfuncs
{

// 如果出错则终止程序
void CloseOrDie(int sockfd);

// 如果出错则终止程序
void BindOrDie(int sockfd, const sockaddr_in * addr);

// 如果出错则终止程序
void ListenOrDie(int sockfd);

// 当发生致命错误时，直接终止程序
// 当发生一般错误时，返回一个负数，上层程序可以忽略这次accept什么也不做
// 当没有错误时，返回连接套接字
// 默认不返回对端的地址信息
int Accept(int sockfd);

// 创建非阻塞式套接字如果出错，则终止程序
// 如果成功返回套接字
int CreateNonBlockSocketOrDie();

// 如果出错则终止程序
void SetReuseAddrOrDie(int fd, int optval);

// 返回当前套接字上发生的错误
int GetSocketError(int sockfd);

// 读半关闭
void ShutDownWrite(int sockfd);

}

}
#endif