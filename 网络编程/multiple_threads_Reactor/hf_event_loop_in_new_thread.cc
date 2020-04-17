#include "hf_event_loop_in_new_thread.h"

namespace hf
{
EventLoopInNewThread::EventLoopInNewThread()
//注意，这里不能使用thread_(Thread(std::bind(&EventLoopInNewThread::ThreadFunc, this))),因为这个是拷贝初始化，
//而Thread是不允许拷贝的
: thread_(std::bind(&EventLoopInNewThread::ThreadFunc, this)),
  event_loop_(NULL),
  mutex_(),
  cond_(mutex_)
{

}

EventLoopInNewThread::~EventLoopInNewThread()
{
    //这里不能判断event_loop_是否为NULL来判断子线程是否终止，因为当子线程结束时，
    // 该指针变成野指针，没有参考价值
    event_loop_->Quit();
}

EventLoop* EventLoopInNewThread::StartLoop()
{
    assert(!thread_.IsStarted());
    thread_.StartThread();

    {
        MutexGuard mutex_guard(mutex_);

        // 等待子线程相关工作完成
        while(event_loop_ == NULL)
        {
            cond_.Wait();
        }
    }

    return event_loop_;
}

void EventLoopInNewThread::ThreadFunc()
{
    EventLoop loop;

    {
        MutexGuard mutex_guard(mutex_);
        event_loop_ = &loop;
        cond_.Notify();
    }
    //printf("before\n");
    loop.Loop();
    //printf("after loop\n");
}

}