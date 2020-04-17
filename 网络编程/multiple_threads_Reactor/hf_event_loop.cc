#include "hf_event_loop.h"

#include <sys/types.h>
#include <unistd.h>
#include <sys/eventfd.h>

#include <cassert>

#include "hf_utils.h"


// 每个线程独有变量，表示当前线程中的EventLoop是谁
// 当前未处于hf命名空间，所以需要加hf
__thread hf::EventLoop* gEventLoop = NULL;
// Poller::Poll函数超时时间
static const int kPollTimeMs = 10000;

// 调用API，创建event_fd
static int CreateEventfd()
{
    int event_fd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if(eventfd < 0)
    {
        hf::ErrorAndQuit("eventfd create error");
    }
    return event_fd;
}

namespace hf
{
EventLoop::EventLoop()
: is_looping_(false), 
  kThreadID_(CurrentThread::GetID()),
  poller_(new Poller()),
  is_quited_(false),
  event_fd_(CreateEventfd()),
  pending_functor_channel_(new Channel(this, event_fd_)),
  is_handling_pending_functors(false)
{
    if(!gEventLoop)
        gEventLoop = this;
    else
        OutputAndQuit("当前线程已经创建过EventLoop对象[EventLoop::EventLoop()]");
    
    // 设置event_fd_的channel
    pending_functor_channel_->SetReadCallBack(std::bind(&EventLoop::HandlerEventfd, this));
    pending_functor_channel_->EnableRead();
}

EventLoop::~EventLoop()
{
    assert(!is_looping_);
    gEventLoop = NULL;
}

// 主循环 
void EventLoop::Loop()
{
    AssertInCreaeteThread();

    assert(!is_looping_);

    // 只有每次poll返回时，才有机会终止循环
    while(!is_quited_)
    {
        // clear()函数并不会导致释放内存，所以速度很快
        activate_channels_.clear();

        is_looping_ = true;

        poller_->Poll(kPollTimeMs, &activate_channels_);

        //Output("poll return");
        for(int i = 0; i < activate_channels_.size(); ++i)
        {
            activate_channels_[i]->HandleEvent();
        }

        // 不在HandlerEventfd()中运行此函数
        // EventLoop::handleRead()只有在调用了EventLoop::wakeup()才能被执行。
        // 如果doPendingFunctors()是在EventLoop::handleRead()内被调用，
        // 那么在IO线程内注册了回调函数并且没有调用EventLoop::wakeup()，那么回调函数不会被立即得到执行，
        // 而必须等待EventLoop::wakeup()被调用后才能被执行。
        DoPengdingFunctors();
    }

    is_looping_ = false;
}


void EventLoop::AssertInCreaeteThread()
{
    if(!IsInCreateThread())
        OutputAndQuit("当前EventLoop对象不在创建线程运行[EventLoop::assertInCreateThread()]");
}

void EventLoop::Update(Channel* p_channel)
{
    AssertInCreaeteThread();

    poller_->UpdateChannels(p_channel);
}

void EventLoop::RemoveChannel(Channel* channel)
{
    AssertInCreaeteThread();
    assert(channel->GetEventLoop() == this);
    poller_->RemoveChannel(channel);
}

// eventfd的可读回调函数，只能由IO线程调用
void EventLoop::HandlerEventfd()
{
    uint64_t temp_one;
    // 感觉这里需要重复读取，否则在相邻两次读之间，调用了多次写，那么将会导致Poller::Poll多次无效的返回
    // 虽然这样对整个程序的正确运行没有影响
    // 答：不需要，每次向event_fd中写数据时，其数是累加的;当读event_fd时，读出的整数是累加后的数，内部被清空为0
    // 所以不能判断temp_one等于1为正确读写，其实可能会大于1
    ssize_t n = ::read(event_fd_, &temp_one, sizeof(temp_one));
    if(n != sizeof(temp_one))
        ErrorAndQuit("read event_fd_ error");

}

void EventLoop::RunInIOThread(const Functor &func)
{
    // 如果当前在IO线程上，则直接运行函数
    if(IsInCreateThread())
        func();
    else
    {
        // 否则添加到pending_functors队列中
        AddPendingFuntor(func);
    }
}

// 添加PendingFuntor，由任何线程调用
void EventLoop::AddPendingFuntor(const Functor &func)
{
        {
            //互斥锁保护pending_functors数据结构
            MutexGuard mutex_gurad(mutex_);
            pending_functors_.push_back(func);
        }

        // 如果不在IO线程中，则肯定需要唤醒
        // 如果在IO线程中，如果此时正在处理pending_functors，则说明在回调函数中又向pending_functors_队列中添加函数
        // 所以需要再次重新唤醒，否则新添加的函数将不能及时的响应
        // 如果不在处理pending_functors，说明在某个事件的回调函数中调用此函数，那么当初完所有回调函数时，会自动调用
        // DoPengdingFunctors函数，所以不需要唤醒
        if(!IsInCreateThread() || is_handling_pending_functors)
            WakeUpEventfd();
}

void EventLoop::WakeUpEventfd()
{
    uint64_t temp_one = 1;
    ssize_t n = ::write(event_fd_, &temp_one, sizeof(temp_one));
    // 暂且不考虑n == 0的情况，一般情况下，write不会因为缓冲区满而返回0
    // 这里也不先检测event_fd_是否可写，因为一般来说，它一定是可以写（按照原本的流程，应该先设置channel是否可写，然后在回调函数中写event_fd_）
    if(n != sizeof(temp_one))
        ErrorAndQuit("write event_fd_ error");
}

void EventLoop::DoPengdingFunctors()
{
    is_handling_pending_functors = true;

    std::vector<Functor> functors;
    {
        MutexGuard mutex_guard(mutex_);
        // 移动语义
        functors = std::move(pending_functors_);
        // 在移动后，原数组内容不敢保证，所以清空一下
        pending_functors_.clear();
    }

    // 注意不要在原数组中直接遍历调用函数，因为那样可能会产生死锁（在回调函数中又调用AddPendingFunctor）
    // 同时还可以减小互斥区长度
    for(int i = 0; i < functors.size(); ++i)
    {
        functors[i]();
    }

    is_handling_pending_functors = false;

}

void EventLoop::Quit()
{ 
    is_quited_ = true; 

    // 利用WakeUpEventfd函数，让Poller::Poll返回，可以让循环立马停止，而不需要等待其返回
    // 如果此时在IO线程中，则当前必然不可能等待在Poller::Poll上，所以可以不掉用WakeUpEventfd
    if(!IsInCreateThread())
        WakeUpEventfd();
}

}