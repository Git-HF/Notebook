#ifndef HF_ACCEPTOR_H_
#define HF_ACCEPTOR_H_

#include "hf_non_copy.h"
#include "hf_channel.h"
#include "hf_socket.h"
#include "hf_event_loop.h"

namespace hf
{

// 对开启服务端的一个封装，主要由TcpServer使用
class Acceptor : public NonCopy
{
    public:
        typedef std::function<void(int connfd)> NewConnectionCallBack;
        // port表示服务器监听的端口号，服务器的默认绑定地址是INADDR_ANY
        Acceptor(EventLoop* event_loop, int port);
        ~Acceptor();

        // Acceptor开始监听
        // 调用此函数之前，应该设置好new_connection_call_back
        void StartListen();

        // 是否正在监听
        bool IsListening() const { return is_listening_; }

       

        void SetNewConnectionCallBack(const NewConnectionCallBack & cb)
        {
            new_connection_call_back_ = cb;
        }

    private:
        EventLoop* event_loop_;
        // 监听套接字
        Socket accept_socket_;
        // 与监听套接字相对应的chennel，用来注册和回调回调函数
        Channel accept_channel_;

        // 是否已经在监听
        bool is_listening_;

        // 当有新的连接到来时，调用此函数
        NewConnectionCallBack new_connection_call_back_;

        // 当监听套接字可读时，由EventLoop回调此函数
        void HandleRead();

};

}
#endif