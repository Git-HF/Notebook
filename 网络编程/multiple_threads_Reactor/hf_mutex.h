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
        Mutex()//: hold_thread_(0) 
        { 
            // 不直接使用assert(pthread_mutex_init(&mutex_, NULL) == 0)
            // 如果assert被注销了，那么里面的函数也不会执行了，锁将失去作用
            int res = pthread_mutex_init(&mutex_, NULL);
            assert(res == 0);
        }

        ~Mutex()
        {
            //assert(hold_thread_ == 0);
            int res = pthread_mutex_destroy(&mutex_);
            assert(res == 0);
        }
        void Lock()
        {
            // 这两个语句顺序不能变
            int res = pthread_mutex_lock(&mutex_);
            assert(res == 0);
            //hold_thread_ = CurrentThread::GetID();
        }

        void Unlock()
        {
            // 这两个语句顺序不能变
            //hold_thread_ = 0;
            int res = pthread_mutex_unlock(&mutex_);
            assert(res == 0);
        }

        pthread_mutex_t* GetMutex() // 不是常量成员函数，因为返回指针，指针可能会修改值
        {
            return &mutex_;
        }


    private:
        pthread_mutex_t mutex_;
        // muduo中有这个成员变量
        // 表示当前持有这个锁的线程ID
        //pid_t hold_thread_;

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

class Condition : public NonCopy
{
    public:
        // 引用类型
        Condition(Mutex &mutex) : mutex_(mutex)
        {
            int res = pthread_cond_init(&cond_, NULL);
            assert(res == 0);
        }

        ~Condition()
        {
            int res = pthread_cond_destroy(&cond_);
            assert(res == 0);
        }

        void Wait()
        {
            int res = pthread_cond_wait(&cond_, mutex_.GetMutex());
            assert(res == 0);
        }

        void NotifyAll()
        {
            int res = pthread_cond_broadcast(&cond_);
            assert(res == 0);
        }

        void Notify()
        {
            int res = pthread_cond_signal(&cond_);
            assert(res == 0);
        }
    
    private:
        // 引用类型
        Mutex &mutex_;
        pthread_cond_t cond_;

};

}

#endif