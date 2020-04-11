#include "hf_poller.h"

#include <cassert>

#include "hf_utils.h"

namespace hf
{
Poller::Poller()
{

}
Poller::~Poller()
{

}

// 运行多路复用的函数
void Poller::Poll(int time_out_ms, ChannelVector* active_channels)
{
    int num_events = ::poll(pollfds_.data(), pollfds_.size(), time_out_ms);
    if(num_events > 0)
    {
        FillActiveChannel(num_events, active_channels);
    }
    else if(num_events < 0)
    {
        ErrorAndQuit("Poller::Poll error");
    }
}

// 添加Poller中监控的文件描述符或者更改已有文件描述符检测事件
void Poller::UpdateChannels(Channel *channel)
{
    //调用该函数之前，已经由上层调用AssertInCreateThread()
    //AssertInEventLoopThread();

    //注意区分fd和idx对应不同数组的索引，注意。
    int fd = channel->GetFD();
    int idx = channel->GetIndex();

    // 修改已经存在的文件描述符
    // 时间复杂度为O(1)
    if(idx > 0)
    {
        // 代码逻辑判断
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);

        //若不关注任何事件，则直接将pollfds_中文件描述符置为负数
        // 如果仅仅只将events置为0,则无法屏蔽POLLERR事件
        if(channel->IsNoneEvent())
        {
            // 通过将pollfd结构中的fd置为负数
            // 改进方法是取相反数再减1
            // 利用idx，加速了查找
            pollfds_[idx].fd = -1;
        }
        else
        {
            assert((pollfds_[idx].fd == fd) || (pollfds_[idx].fd == -1));
            // 可能之前被置为负数，现在需要改过来
            if(pollfds_[idx].fd < 0)
            {
                pollfds_[idx].fd = fd;
            }
            
            //更新关注的事件
            pollfds_[idx].events = static_cast<short>(channel->GetEvents());
            //revents每次返回时由内核修改，所以我们不用操作
        }
    }
    else
    {
        // 添加新的Channel时间复杂度为O(logN)，因为在map中插入一个元素时间复杂度是O(logN)
        assert(channels_.find(fd) == channels_.end());
        
        //更新pollfds_数组
        pollfd pfd;
        pfd.fd = fd;
        pfd.events = static_cast<short>(channel->GetEvents());
        // pfd.revents = 0;
        pollfds_.push_back(pfd);

        //反向更新Channel的index_成员
        idx = static_cast<int>(pollfds_.size() - 1);
        channel->SetIndex(idx);

        //更新channels_成员
        //这里不是idx
        channels_[fd] = channel;

    }
}

void Poller::FillActiveChannel(int num_events, ChannelVector* active_channels) const
{
    // 是否在IO线程中
    // 不需要判断，因为该函数为私有成员，只有Poller本身才可以调用，而Poller本身一定处在IO线程中。
    //AssertInEventLoopThread();

    PollfdVector::const_iterator pfd = pollfds_.cbegin();
    
    for(; (pfd != pollfds_.cend() && (num_events > 0)); ++pfd)
    {
        if(pfd->revents > 0)
        {
            // 从map中找到与当前pollfd相对应的Channel*，
            // 这里就体现出使用map结构的好处，如果不使用map，这里查找的复杂度应该是O(n)
            FdToChannelMap::const_iterator tmp_iter = channels_.find(pfd->fd);
            assert(tmp_iter != channels_.cend());

            Channel* channel = tmp_iter->second;
            assert(channel->GetFD() == pfd->fd);

            channel->SetRevents(pfd->revents);
            active_channels->push_back(channel);
            
            // 利用num_events提前减到0,提前结束
            --num_events;
        }
    }
}

}