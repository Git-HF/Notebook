#include "hf_channel.h"

#include <poll.h>

#include <cassert>

#include "hf_event_loop.h"
#include "hf_utils.h"


namespace hf
{

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;


Channel::Channel(EventLoop* event_loop, int fd)
: event_loop_(event_loop),
    fd_(fd),
    events_(kNoneEvent),
    revents_(kNoneEvent),
    index_(-1),
    eventHanding_(false)
{

}

Channel::~Channel()
{
    assert(!eventHanding_);
}

void Channel::HandleEvent()
{
    eventHanding_ = true;

    // 关于这一部分，还可以再好好研究一下
    // 对于Acceptor来说，基本只需要处理可读状态，其他回调函数没有设置，
    // 对于TcpConnection来说，基本四种情况都需要设置

    //异常发生
    if(revents_ & (POLLERR | POLLNVAL)){
        //printf("Channel::HandleEvent except_call_bakc\n");
        //assert(except_call_back_);
        if(except_call_back_)
        {
            except_call_back_();
        }
    }

    // 对方Close
    // 关于POLLHUP和POLLRDHUP详见博客
    // 感觉第二个参数
    // -------------------------------------这个语句好像一般不会执行-----------------------------------
    if((revents_ & POLLHUP) && !(revents_ & POLLIN))
    {
        if(close_call_back_)
        {
            //printf("close_call_back()\n");
            close_call_back_();
        }
    }

    // 可读
    // 最后一个参数是POLLRDHUP，不是POLLHUP;它表示对端关闭连接（shutdown）
    if(revents_ & (POLLIN | POLLPRI | POLLRDHUP)){
        if(read_call_back_)
        {
            //printf("Channel::HandleEvent read_call_back\n");
            read_call_back_();
        }
    }

    //可写
    if(revents_ & POLLOUT){
        //printf("Channel::HandleEvent write__call_back\n");

        if(write_call_back_)
        {
            write_call_back_();
        }
    }

    eventHanding_ = false;
}


void Channel::Update(){
    event_loop_->Update(this);
}



}