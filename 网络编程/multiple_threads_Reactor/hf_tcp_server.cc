#include "hf_tcp_server.h"

namespace hf
{
TcpServer::TcpServer(EventLoop* loop, int port, int thread_pool_arg)
: event_loop_(loop),
  acceptor_(event_loop_, port),
  started_(false),
  thread_poll_(loop, thread_pool_arg)
{
    // 注意参数std::placeholders::_1的设置
    acceptor_.SetNewConnectionCallBack(std::bind(&TcpServer::NewConection, this, std::placeholders::_1));
}

TcpServer::~TcpServer()
{

}

// 其实服务器启动就让监听套接字进入listen状态，然后开启相应套接字的读事件标志
// 其实还应该让EventLoop进入poll函数，目前版本是在用户代码中写的
void TcpServer::StartServer()
{
    assert(!started_);
    started_ = true;

    assert(!acceptor_.IsListening());
    // 不太理解为啥只能在IO线程中调用Listen？？？
    // 看笔记
    event_loop_->RunInIOThread(std::bind(&Acceptor::StartListen, &acceptor_));
}
    
void TcpServer::NewConection(int connfd)
{
    // 这个函数实际调用过程是从IO函数开始的，所以一定在IO线程中
    event_loop_->AssertInCreaeteThread();

    EventLoop* io_loop = thread_poll_.GetNextEventLoop(); 
    // 当有新连接时，建立TcpConnectionPtr
    // 关于智能指针的初始化，可以考虑使用make_shared，详见收藏的书签
    TcpConnectionPtr conn(new TcpConnection(io_loop, connfd));
    // 将新连接加入集合
    connections_.insert(conn);

    //设置回调函数
    conn->SetMessageCallBack(message_call_back_);
    conn->SetCloseCallBack(std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));

    // 在自己所归属的线程进行TcpConnection的相关初始化
    io_loop->RunInIOThread(bind(&TcpConnection::EstablishedTcpConnection, conn));

    // 如果用户设置了该函数则调用
    // if(connection_call_back_)
    //    connection_call_back_(conn);
}

void TcpServer::RemoveConnection(const TcpConnectionPtr & conn)
{

    // 由于TcpServer中connections_是没有锁的，所以任何修改connections_结构都必须在IO线程中执行
    event_loop_->RunInIOThread(std::bind(&TcpServer::RemoveConnectionInLoop, this, conn));
    
}

void TcpServer::RemoveConnectionInLoop(const TcpConnectionPtr &conn)
{
    event_loop_->AssertInCreaeteThread();

    size_t n = connections_.erase(conn);
    assert(n == 1);

    // 延长TcpConnetion寿命的关键步骤，延长到EventLoop对象执行完DestroyTcpConneciton函数
    // 注意这里不能是event_loop->RunInIOThread
    // 在多线程的场景下，保存TcpConnection对象的是TcpServer中的connections_，所以删除其中的TcpConnection需要在主线程中执行
    // 而保存监测TcpConnection的可读可写事件的是子线程中的EventLoop，所以最终删除Poller中的相关监测结构需要在子线程中执行
    EventLoop* loop = conn->GetEventLoop();
    loop->AddPendingFuntor(std::bind(&TcpConnection::DestroyedTcpConnection, conn));

}

}