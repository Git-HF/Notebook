#include "hf_thread.h"

#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cassert>

#include "hf_utils.h"


namespace
{
    // 每个线程的唯一标识，每个线程独有
    // 0是一个非法值，操作系统第一个进程init的pid为1
    __thread pid_t gThreadID = 0;
}

namespace hf
{
    namespace CurrentThread
    {
    
    pid_t GetID()
    {
        // 每个线程第一次调用时初始化
        if(gThreadID == 0)
            gThreadID = ::syscall(SYS_gettid);       //syscall必须同时包含<sys/syscall.h>和<unistd.h>
        return gThreadID;
    }

    }

    namespace threadhelp
    {
        struct ThreadData
        {
            ThreadData(const Thread::ThreadFunc &func) : func_(func) { }
            void Run() { func_(); }
            Thread::ThreadFunc func_;
        };
    }

void* StartThreadHelp(void* args)
{
    threadhelp::ThreadData* thread_data = static_cast<threadhelp::ThreadData*>(args);
    thread_data->Run();
}

Thread::Thread(const ThreadFunc & func)
: thread_func_(func),
  thread_ID_(0),
  started_(false),
  join_(false)
{

}

Thread::~Thread()
{
    // 如果线程启动了，
    // 当这个线程类启动的线程结束了，这个函数会返回错误，但是对事实没有影响，所以也不检测pthread_detach是否返回错误(好像会产生内存泄漏？)
    // 如果没有结束，那么pthread_detach将使其为分离状态，不会产生内存泄漏
    if(started_ && join_)
        pthread_detach(thread_ID_);
}

//启动线程 
void Thread::StartThread()
{
    // 必须没有启动过
    assert(!started_);
    // 目前这里有问题，如果新线程结束了，会造成内存泄漏？？？
    threadhelp::ThreadData* thread_data = new threadhelp::ThreadData(thread_func_);
    assert(thread_data);
    int res = pthread_create(&thread_ID_, NULL, &StartThreadHelp, (void*)thread_data);

    if(res > 0)
    {
        started_ = false;
        delete thread_data;
        OutputAndQuit("create thread error");
    }
}

}