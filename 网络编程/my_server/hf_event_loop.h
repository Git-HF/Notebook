#ifndef HF_EVENTLOOP_H_
#define HF_EVENTLOOP_H_

#include <sys/types.h>      //pid_t

#include <memory>           //unique_ptr
#include <vector>
#include <functional>

#include "hf_non_copy.h"
#include "hf_thread.h"      //CurrentThread::gettid()
#include "hf_channel.h"
#include "hf_poller.h"
#include "hf_mutex.h"

namespace hf
{
//继承NonCopy，防止EventLoop拷贝，当时可以使用指针或者引用
class EventLoop : public NonCopy
{
    typedef Poller::ChannelVector ChannelVector;

    public:

        typedef std::function<void(void)> Functor;

        EventLoop();
        ~EventLoop();

        // 主循环 
        void Loop();

        //内联函数
        //当前的EventLoop对象是否运行在创建自己的线程上
        bool IsInCreateThread() const
        {
            return (kThreadID_ == CurrentThread::GetID());
        }

        // 判断当前EventLoop对象是否在运行在创建线程上，如果不在则退出
        // const函数？？？
        void AssertInCreaeteThread();

        // 由Channel对象调用，用来更新相应描述符上关心的事件
        void Update(Channel* channel);

        // 停止事件循环
        void Quit();

        // eventfd的可读回调函数，只能由IO线程调用
        void HandlerEventfd();
        // 在IO线程(即每一个创建EventLoop对象的线程)中运行函数func，这样有些数据结构就不用加锁，保证安全
        void RunInIOThread(const Functor &func);

    private:
        //创建该事件循环的线程
        const pid_t kThreadID_;
        bool is_looping_;

        // 每次Poller::Poll函数返回时，返回激活的描述负符
        ChannelVector activate_channels_;

        std::unique_ptr<Poller> poller_;

        // 是否结束
        bool is_quited_;

        int event_fd_;

        // 函数列表，表示这些函数必须在IO线程中执行
        std::vector<Functor> pending_functors_;

        // 在EventLoop，别的线程利用eventfd来通知IO线程我在pendging_functors中添加了一个函数，需要待执行
        // 该channel就是对eventfd的封装，当该文件描述符可读时，停止Poll的等待，执行pending_functors中的函数
        std::unique_ptr<Channel> pending_functor_channel_;
        // 由于其它线程会更新此数据结构，所以需要对其加锁
        Mutex mutex_;

        // 向eventfd中写一个数据，使其可读，这样poll才能返回，运行pending_functors中的函数
        void WakeUpEventfd();
        // 添加PendingFuntor，由任何线程调用
        void AddPendingFuntor(const Functor &func);
        // 处理pending_functors_中的函数
        void DoPengdingFunctors();

        bool is_handling_pending_functors;
};
}


#endif