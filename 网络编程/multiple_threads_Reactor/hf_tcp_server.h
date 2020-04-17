#ifndef HF_TCP_SERVER_H_
#define HF_TCP_SERVER_H_

#include <set>

#include "hf_acceptor.h"
#include "hf_event_loop.h"
#include "hf_tcp_connection.h"
#include "hf_event_loop_thread_pool.h"

namespace hf
{

// 由用户直接调用，生命期由用户控制
class TcpServer
{
    typedef std::function<void(const TcpConnectionPtr&)> ConnectionCallBack;

    public:
        // port 表示服务器的监听窗口
        TcpServer(EventLoop* loop, int port, int thread_pool_arg);
        ~TcpServer();

        // 启动服务器，线程安全
        void StartServer();
        
        // 不是线程安全的
        void SetMessageCallBack(const MessageCallBack &cb)
        {
            message_call_back_ = cb;
        }

        // 不是线程安全的
        void SetConnectionCallBack(const ConnectionCallBack &cb)
        {
            connection_call_back_ = cb;
        }


    private:
        // 感觉TcpServet是最顶层的类了，所以可以在这个类里面创建Eventloop对象，而不需要在用户代码里创建Eventloop对象
        EventLoop* event_loop_;
        Acceptor acceptor_;

        // 源码里使用map，还没感受到其中的好处
        // 这里可以考虑到底使用set还是unorder_set
        // TcpConnectionPtr的生命期是模糊的，其持有者可能是TcpConnectionPtr，也可能是用户（在OnConnection和OnMessage函数中）
        std::set<TcpConnectionPtr> connections_;

        bool started_;

        // 还不知道干嘛用的
        // 新建连接时，用以生成新连接的name
        // int nextConnId_;

        // 线程池，如果线程池参数大小为0,则所有操作都在主线程中完成
        // 否则主线程利用accept获取新连接，传递给线程池中的线程
        EventLoopThreadPool thread_poll_;

        // 当有数据到达时，TcpConnection回调此函数。先由用户设置此函数，然后需要将此函数设置给TcpConnection
        MessageCallBack message_call_back_;

        // 当建立新连接时，回调此函数
        // 由于不是所有情况下都是客户端先发数据，也可能服务端先发数据，所以需要提供该接口
        // 如果用户未设置该选项，则不执行
        // 在源码中，该函数是由TcpConnection回调的
        ConnectionCallBack connection_call_back_;

        // 当有新连接到来时，由Acceptor回调此函数; 一开始需要将此函数设置给Acceptor
        void NewConection(int connfd);

        void RemoveConnection(const TcpConnectionPtr & conn);

        void RemoveConnectionInLoop(const TcpConnectionPtr &conn);
};

}
#endif
