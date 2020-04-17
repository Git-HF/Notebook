#ifndef HF_TCP_CONNECTION_H_
#define HF_TCP_CONNECTION_H_

#include <memory>

#include "hf_socket.h"
#include "hf_non_copy.h"
#include "hf_channel.h"
#include "hf_event_loop.h"
#include "hf_buffer.h"

namespace hf
{
class TcpConnection;

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

typedef std::function<void(const TcpConnectionPtr&, Buffer *buf)> MessageCallBack;

typedef std::function<void(const TcpConnectionPtr&)> CloseCallBack;

// 一个TcpConnection表示一次Tcp连接，本类中只保存了连接套接字，并未保存对端地址
// 继承std::enable_shared_from_this为了可以正确返回指向该对象的智能指针
// 如果不继承，可能会造成智能指针引用计数错误，造成多次释放内存，详细内容搜博客
// TcpConnection默认开启是否可读的事件检测
class TcpConnection :public NonCopy, public std::enable_shared_from_this<TcpConnection>
{

    public:
        enum StateE { kConnecting, kConnected, kDisconnecting, kDisconnected};

        TcpConnection(EventLoop* loop, int fd);
        ~TcpConnection();

        void SetMessageCallBack(const MessageCallBack & cb)
        {
            message_call_back_ = cb;
        }

        void SetCloseCallBack(const CloseCallBack & cb)
        {
            close_call_back_ = cb;
        }

        bool IsConnected() const { return state_ == kConnected; }

        EventLoop* GetEventLoop() const { return event_loop_; }

        // 当连接建立时，应该通知上层
        // 因为服务端可能不需要读取客户端数据，而直接向客户端发送数据，例如Daytime服务。
        //void ConnectionEstablished();
        void SetState(StateE s) { state_ = s; }

        // 用于移除一个TcpConnection的收尾工作，清除Poller中监听的描述符
        // 每个连接只被调用一次
        void DestroyedTcpConnection();
        
        // 当新建连接时，调用此函数。每个新建连接只被调用一次
        // 必须存在的原因：在多线程环境下，TcpConnection的建立在主线程，而接下来的对于该TcpConnection的可读可写监听
        // 在子线程中，所以关于TcpConnection的初始化（开启可读事件监听）工作应该放在子线程中，所以需要此函数
        void EstablishedTcpConnection();

        // 主动关闭连接
        // 主动关闭连接调用ShutDown，暂时不要调用ForceClose
        // void ForceClose();

        // 线程安全
        // 为啥要确保发送数据的线程安全？
        // 答：因为发送数据可能会修改out_buffer_,所以需要保证线程安全
        void SendData(const std::string & data);

        // 读半关闭
        // 线程安全
        // 为啥要保证shutdown的线程安全？？或者说为啥一定要在IO线程中执行这个函数
        // 如果不保证线程安全，那么当别人正在写数据时，调用该函数将会发生错误。如果都在IO线程中执行，
        // 那么除非用户是先调用ShutDonw,再调用SendData，否则不会出现上面的情况
        void ShutDown();

    private:

        EventLoop* event_loop_; 
        // 当TcpConnection对象析构时，会调用Socket对象的析构函数，其中关闭文件描述符号，注意不要重复关闭
        Socket conn_socket_;
        Channel conn_channel_;

        // 由于TcpConnection需要在线程中共享，可能多个线程修改该变量，所以最好将其定义为原子变量。。。。
        // 定义对state_的操作均为原子操作
        StateE state_;

        // 输入输出是相对于用户来说的
        // 输入缓冲区表示用户从这里读数据，而Buffer需要往这里写数据
        Buffer intput_buffer_;

        // 输出缓冲区表示用户往这里面写数据，而Buffer需要从这里面读数据
        Buffer output_buffer_;


        // 当有数据到达时，回调此函数，通知上层，也就是TcpServer
        MessageCallBack message_call_back_;

        // 当需要关闭连接时，回调此函数，通知上层，也就是TcpServer
        CloseCallBack close_call_back_;

        // 本类表示一次Tcp连接，当这个类第一次建立时，调用此回调函数，告知上层用户，有新的连接建立
        // 上层用户可能在不需要读取客户端数据的情况下向其发送数据，所以需要此函数的存在
        //ConnectionCallBack connection_call_back_;

        // 当套接字可读时，由Channel回调此函数
        void HandleRead();

        // 该函数可由HandleRead()调用，或者由Channel回调
        // 目前来说，该函数只能被调用一次，调用多次会出错
        void HandleClose();

        void HandleWrite();

        void HandleError();

        // 只能在IO线程中执行以下函数
        // void ForceCloseInLoop();
        void SendDataInLoop(const std::string & data);
        void ShutDownInLoop();
};


}

#endif