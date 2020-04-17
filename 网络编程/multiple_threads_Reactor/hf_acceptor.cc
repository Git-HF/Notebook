#include "hf_acceptor.h"

namespace hf
{
Acceptor::Acceptor(EventLoop* event_loop, int port)
: event_loop_(event_loop),
  accept_socket_(socketfuncs::CreateNonBlockSocketOrDie()),
  accept_channel_(event_loop, accept_socket_.Getfd()),
  is_listening_(false)
{
    // 默认开启SO_REUSEADDR标志
    accept_socket_.SetReuse(true);

    // 设置默认服务器地址和端口
    sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    accept_socket_.BindAddress(&server_addr);

    accept_channel_.SetReadCallBack(std::bind(&Acceptor::HandleRead, this));
}

Acceptor::~Acceptor()
{

}

// Acceptor开始监听
void Acceptor::StartListen()
{
    assert(is_listening_ == false);
    // 只能在IO线程中监听
    event_loop_->AssertInCreaeteThread();

    // 调用当前函数之前应该先设置好新连接的回调函数
    assert(new_connection_call_back_);

    is_listening_ = true;

    accept_socket_.Listen();
    accept_channel_.EnableRead();
}


void Acceptor::HandleRead()
{
    event_loop_->AssertInCreaeteThread();

   // 对于并发量比较大的时候，这里应该一直accept,直到将所有连接都获取完了
   // 不然Poller::Poll将返回多次，浪费性能
    int connfd = accept_socket_.Accept();
    // 如果小于0,说明是不重要的错误，什么也不做
    if(connfd >= 0)
    {
        assert(new_connection_call_back_);
        // 这种传递文件描述符的做法不是很理想，可以使用移动语义，传递Socket结构
        new_connection_call_back_(connfd);
    }

}


}