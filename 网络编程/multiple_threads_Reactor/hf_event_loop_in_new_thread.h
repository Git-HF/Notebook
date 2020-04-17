#ifndef HF_EVENT_LOOP_IN_NEW_THREAD_H_
#define HF_EVENT_LOOP_IN_NEW_THREAD_H_

#include "hf_event_loop.h"
#include "hf_thread.h"


namespace hf
{
// 先创建一个新线程，然后在新线程中创建EventLoop
class EventLoopInNewThread : public NonCopy
{
    public:
        EventLoopInNewThread();
        ~EventLoopInNewThread();

        // 真正启动线程
        EventLoop* StartLoop();

    private:
        void ThreadFunc();

        Thread thread_;
        // 从后面函数可以看出，该变量指向一个函数中的临时变量，如果函数结束，那么他将变成野指针
        EventLoop* event_loop_;
        // 主要用来保护条件变量
        Mutex mutex_;
        // 为了等待新线程
        Condition cond_;
};


}
#endif