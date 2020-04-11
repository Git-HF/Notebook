#ifndef HF_CHANNEL_H_
#define HF_CHANNEL_H_

#include <functional>       //std::function

#include "hf_non_copy.h"


namespace hf
{

//如果这个前向申明写在命名空间外面，那么必须要加hf::EventLoop，否则编译器以为他是一个新类型
// 因为EventLoop类型和hf::EventLoop类型是不同的
//前向声明
class EventLoop;

// Channel对象自始至终只负责一个文件描述符，只属于一个EventLoop对象
// 主要完成IO事件的分发，它不打开也不关闭文件描述符
// 其功能主要是设置文件描述关注的事件和相应的回调函数
// Channel的成员函数一般只能在IO线程中调用，所以更新成员不必加锁？？？？
// 真正拥有Channel对象的只有Acceptor类和TcpConnection类，其他类都只是保存了指针。
class Channel : public NonCopy
{
    public:
        typedef std::function<void(void)> EventCallBackFunc;

        Channel(EventLoop* event_loop, int fd);

        //参数为常量引用
        void SetReadCallBack(const EventCallBackFunc& read_cb){
            read_call_back_ = read_cb;
        }

        void SetWriteCallBack(const EventCallBackFunc& write_cb){
            write_call_back_ = write_cb;
        }

        void SetExcpetCallBack(const EventCallBackFunc& except_cb){
            except_call_back_ = except_cb;
        }

        void EnableRead(){
            events_ |= kReadEvent;
            Update();
        }

        void EnableWrite(){
            events_ |= kWriteEvent;
            Update();
        }
        
        void DisableRead(){
            events_ |= (~kReadEvent);
            Update();
        }
        void DisableWrite(){
            events_ |= (~kWriteEvent);
            Update();
        }

        //当IO多路复用函数返回时，由其他类对象回调此函数，判断发生的事件
        void HandleEvent();

        int GetIndex() const { return index_; }
        int GetFD() const { return fd_; }
        int GetEvents() const { return events_; }

        // 调用这两个函数都是在IO线程中，所以数据更新不需要加锁

        void SetIndex(int idx) { index_ = idx; }

        // poll -> FillActivateChannels -> SetRevents，在IO线程中
        void SetRevents(int revents) { revents_ = revents; }


        // 判断当前描述符关注的事件为kNoneEvent，也就是不关注任何事件
        bool IsNoneEvent() const { return events_ == kNoneEvent; }

    private:
        // 当设置可读、可写标志时，需要更新Poller对象中的相应参数
        // 调用流程是update调用EventLoop对象的Update; EventLoop对象的Update调用Poller对象的Update
        // 该函数只在IO线程中调用，所以不用加锁;
        void Update();

        EventLoop* event_loop_;

        // const类型
        const int fd_;

        // 文件描述符关注的事件，本身是short类型，为啥不直接定义成short类型，后面还要强制类型转换？？？
        int events_;
        // 文件描述符发生的事件
        int revents_;
        // ？？？具体作用不清楚
        // 表示Poller中数组的序号，通过这个序号，在Poller中修改和删除时可以加快速度
        // 该序号初始化为-1，当加入Poller中检测数组时真正修改
        int index_;

        // 当文件描述符可读，可写，发生异常时的回调函数
        EventCallBackFunc read_call_back_, write_call_back_, except_call_back_;

        static const int kNoneEvent;
        static const int kReadEvent;
        static const int kWriteEvent;
        
};
}

#endif