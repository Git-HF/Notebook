#ifndef HF_MUTEX_H_
#define HF_MUTEX_H_

// 主要参考muduo中Mutex的实现，但是那个有点复杂，没太看懂
#include <pthread.h>

#include <assert.h>

#include "hf_non_copy.h"
#include "hf_thread.h"

namespace hf
{
class Mutex : public NonCopy
{
    public:
        Mutex(): hold_thread_(0) 
        { 
            // 不直接使用assert(pthread_mutex_init(&mutex_, NULL) == 0)
            // 如果assert被注销了，那么里面的函数也不会执行了，锁将失去作用
            int res = pthread_mutex_init(&mutex_, NULL);
            assert(res == 0);
        }

        ~Mutex()
        {
            assert(hold_thread_ == 0);
            int res = pthread_mutex_destroy(&mutex_);
            assert(res == 0);
        }
        void Lock()
        {
            // 这两个语句顺序不能变
            int res = pthread_mutex_lock(&mutex_);
            assert(res == 0);
            hold_thread_ = CurrentThread::GetID();
        }

        void Unlock()
        {
            // 这两个语句顺序不能变
            hold_thread_ = 0;
            int res = pthread_mutex_unlock(&mutex_);
            assert(res == 0);
        }


    private:
        pthread_mutex_t mutex_;
        // 表示当前持有这个锁的线程ID
        pid_t hold_thread_;

};

class MutexGuard
{
    public:
        // 参数是引用参数
        MutexGuard(Mutex &mutex) : mutex_(mutex)
        {
            mutex_.Lock();
        }

        ~MutexGuard()
        {
            mutex_.Unlock();
        }

    private:
        // 这里是一个引用，注意
        Mutex& mutex_;

};
}

#endif