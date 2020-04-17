#ifndef HF_THREAD_
#define HF_THREAD_

#include <sys/types.h>      //pid_t

#include <functional>

#include "hf_non_copy.h"

namespace hf
{
    namespace CurrentThread
    {
    // 返回当前线程的唯一标识
    pid_t GetID();

    }

// 简单版本的线程类，主要是对pthread的封装
// 这个类不太完全，还有很多问题
class Thread : public NonCopy
{
    public:
        // 这里不定义成void*(void*)是因为一个类的非静态成员函数是无法传递给pthread_create
        // 因为非静态成员函数必须和对象绑定在一起，只有通过对象类对象才能调用
        // 所以为了给pthread_create传递参数，我们必须再做一次封装，详情见ThreadData
        typedef std::function<void(void)> ThreadFunc;

        Thread(const ThreadFunc & func);
        ~Thread();

        // 启动线程 
        void StartThread();

        bool IsStarted() const { return started_; }

    private:

        //static void* ProxyThreadFunc(void* args);
        ThreadFunc thread_func_;
        // pthread_self()的返回值
        pthread_t thread_ID_;
        // 线程是否已经开启
        bool started_;
        // 是否为join状态，默认是detach状态
        bool join_;

};

}
#endif