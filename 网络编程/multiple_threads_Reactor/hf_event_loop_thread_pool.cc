#include "hf_event_loop_thread_pool.h"

namespace hf
{
EventLoopThreadPool::EventLoopThreadPool(EventLoop* loop, int num_threads)
: base_loop_(loop),
  num_threads_(num_threads),
  next_thread_index_(0)
{
    // 默认直接启动线程池中的线程
    Start();
}

EventLoopThreadPool::~EventLoopThreadPool()
{

}

EventLoop* EventLoopThreadPool::GetNextEventLoop()
{
    base_loop_->AssertInCreaeteThread();

    EventLoop* event_loop = base_loop_;

    // 如果线程池不为空，则返回线程池中的一个线程，否则返回主线程
    if(!event_loop_pools_.empty())
    {
        event_loop = event_loop_pools_[next_thread_index_];
        ++next_thread_index_;
        assert(next_thread_index_ <= num_threads_);
        next_thread_index_ = (next_thread_index_ == num_threads_ ? 0 : next_thread_index_);
    }

    return event_loop;
}

void EventLoopThreadPool::Start()
{
    base_loop_->AssertInCreaeteThread();

    // 创建线程并启动
    for(int i = 0; i < num_threads_; ++i)
    {
        // 创建一个新线程
        thread_pools_.push_back(std::unique_ptr<EventLoopInNewThread>(new EventLoopInNewThread()));
        // 启动新线程
        event_loop_pools_.push_back(thread_pools_[i]->StartLoop());
    }
}
}