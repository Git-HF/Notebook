#include "hf_tcp_connection.h"

#include <unistd.h>

namespace hf
{
TcpConnection::TcpConnection(EventLoop* loop, int fd)
: event_loop_(loop),
  conn_socket_(fd),
  conn_channel_(loop, fd),
  state_(kConnecting)
{
    conn_channel_.SetReadCallBack(std::bind(&TcpConnection::HandleRead, this));
    conn_channel_.SetWriteCallBack(std::bind(&TcpConnection::HandleWrite, this));
    conn_channel_.SetCloseCallBack(std::bind(&TcpConnection::HandleClose, this));
    conn_channel_.SetExcpetCallBack(std::bind(&TcpConnection::HandleError, this));
}

TcpConnection::~TcpConnection()
{
    assert(state_ == kDisconnected);
}

/*
void TcpConnection::ConnectionEstablished()
{
    // 这条语句主要是防止用户错误使用代码，因为按照本来的代码逻辑，应该不会出现这样的错误
    event_loop_->AssertInCreaeteThread();
    assert(state_ == kConnecting);
    SetState(kConnected);

    // 默认开启连接套接字的是否的可读标志
    conn_channel_.EnableRead();
}
*/

void TcpConnection::HandleRead()
{
    int saved_error = 0;
    // 未处理出错情况
    ssize_t n = intput_buffer_.ReadFd(conn_socket_.Getfd(), &saved_error);

    if(n > 0)
    {
        assert(message_call_back_);
        // shared_from_this：返回指向该对象的智能指针
        message_call_back_(shared_from_this(), &intput_buffer_);
    }
    else if(n == 0)
    {
        HandleClose();
    }
    else
    {
        // 这个语句的必要性还不清楚？？？
        errno = saved_error;
        // 处理出错的情况
        HandleError();
    }
    
}

void TcpConnection::HandleClose()
{
    event_loop_->AssertInCreaeteThread();

    assert(state_ == kConnected || state_ == kDisconnecting);

    // 源码中调用了该语句
    // 感觉主要是在ForceClose中起作用
    conn_channel_.DisableAll();

    assert(close_call_back_);
    close_call_back_(shared_from_this());
}

void TcpConnection::HandleWrite()
{
    event_loop_->AssertInCreaeteThread();

    if(conn_channel_.IsWriteEvent())
    {
        ssize_t n = ::write(conn_socket_.Getfd(), 
                            output_buffer_.FirstReadablePos(), 
                            output_buffer_.ReadableBytes());
        // 这里不检测出错的情况，如果套接字出错了，由于每个连接套接字默认都是开启读事件的，
        // 那么读事件将返回0或者出错。
        if(n > 0)
        {
            output_buffer_.Retrieve(n);
            // 如果没有要发送的数据，则需要关闭写事件，否则水平触发的方式，将导致poll一直返回
            if(output_buffer_.ReadableBytes() == 0)
            {    
                conn_channel_.DisableWrite();
                // 这里可以直接调用ShutDownInLoop的原因是前面已经保证这个线程就是IO线程，所以可以直接调用
                // 否则需要调用Shutdonw()
                if(state_ == kDisconnecting)
                    ShutDownInLoop();
            }
        }
        else if(n == 0)
        {
            Output("TcpConnection::HandleWrite write返回0");
        }
        else
        {
            Output("TcpConnection::HandleWrite write返回-1");
        }
        
    }
    else
    {
        Output("TcpConnection Handle: 写标志未开启");
    }
    

}

void TcpConnection::HandleError()
{
    // int err = socketfuncs::GetSocketError(conn_channel_.GetFD());

    OutputError("[TcpConnection::HandleError] error");
    // 如果相应套接字发生错误，应该关闭套接字
}

void TcpConnection::DestroyedTcpConnection()
{
    event_loop_->AssertInCreaeteThread();

    assert(state_ == kConnected || state_ == kDisconnecting);
    SetState(kDisconnected);

    // 更新Poller中监听的描述符
    event_loop_->RemoveChannel(&conn_channel_);
}

void TcpConnection::EstablishedTcpConnection()
{
    event_loop_->AssertInCreaeteThread();
    assert(state_ == kConnecting);
    SetState(kConnected);
    conn_channel_.EnableRead();

    // 源码中还回调了新建连接的回调函数
}

/*
void TcpConnection::ForceClose()
{
    // 源码中这里使用if判断？？？
    // 这里可以等于kDisconnecting是因为可能在调用ShutDown后调用ForceClose，这种调用逻辑应该时正确的
    assert(kConnected == state_ || kDisconnecting == state_);

    SetState(kDisconnecting);
    event_loop_->RunInIOThread(std::bind(&TcpConnection::ForceCloseInLoop, this));
}

void TcpConnection::ForceCloseInLoop()
{
    event_loop_->AssertInCreaeteThread();

    assert(kConnected == state_);

    HandleClose();
}
*/

void TcpConnection::SendData(const std::string & data)
{
    // 为什么用if，为啥不用assert，这里不应该一定是kConnected，如果不是则退出？？？
    assert(state_ == kConnected);
    if(state_ == kConnected)
    {
        if(event_loop_->IsInCreateThread())
            SendDataInLoop(data);
        else
        {
            event_loop_->RunInIOThread(std::bind(&TcpConnection::SendDataInLoop, this, data));
        }
    }
    
}

// 线程安全
void TcpConnection::ShutDown()
{
    // 为啥不用assert？？？
    // 为了多个线程中，重复调用ShutDown不会出错
    assert(state_ == kConnected);
    if(state_ == kConnected)
    {
        SetState(kDisconnecting);
        event_loop_->RunInIOThread(std::bind(&TcpConnection::ShutDownInLoop, this));
    }
}

void TcpConnection::SendDataInLoop(const std::string & data)
{
    event_loop_->AssertInCreaeteThread();

    ssize_t n = 0;
    // 这个ReadableByte() == 0表示没有待发送的数据
    if(!conn_channel_.IsWriteEvent() && (output_buffer_.ReadableBytes() == 0))
    {
        n = ::write(conn_socket_.Getfd(), data.data(), data.size());
        if(n < 0)
        {
            n = 0;
            // 发生致命性错误，则退出
            // 是否需要判断被中断的系统调用错误？？
            // 好像不用，因为非阻塞不会等待
            if(errno != EWOULDBLOCK)
                ErrorAndQuit("TcpConnection::SendDataInLoop write error");
        }
    }
    
    assert(n >= 0);
    // 有尚未发送完的数据，需要开启读事件检测
    if(static_cast<ssize_t>(n) < data.size())
    {
        // 怎么利用利用C++移动语义，避免内存拷贝？？
        output_buffer_.AppendStr(data.data() + n, data.size() - n);
        if(!conn_channel_.IsWriteEvent())
            conn_channel_.EnableWrite();

    }
}

void TcpConnection::ShutDownInLoop()
{
    event_loop_->AssertInCreaeteThread();

    // 如果还有正在写数据，由于在ShutDown中已经设置了标志位，那么将在数据写完时，执行读半关闭。
    if(!conn_channel_.IsWriteEvent())
    {
        socketfuncs::ShutDownWrite(conn_socket_.Getfd());
    }
}

}