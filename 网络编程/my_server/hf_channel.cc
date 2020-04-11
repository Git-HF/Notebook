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
    index_(-1)
{

}

void Channel::HandleEvent(){

    //异常发生
    if(revents_ & (POLLERR | POLLNVAL)){
        assert(except_call_back_);
        except_call_back_();
    }

    // 可读
    // 最后一个参数是POLLRDHUP，不是POLLHUP;它表示对端关闭连接
    if(revents_ & (POLLIN | POLLPRI | POLLRDHUP)){
        assert(read_call_back_);
        read_call_back_();
    }

    //可写
    if(revents_ & POLLOUT){
        assert(write_call_back_);
        write_call_back_();
    }
}


void Channel::Update(){
    event_loop_->Update(this);
}



}