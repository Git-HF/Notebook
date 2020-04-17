#ifndef HF_EVENT_LOOP_THREAD_POOL_H_
#define HF_EVENT_LOOP_THREAD_POOL_H_

#include <memory>
#include <vector>

#include "hf_event_loop_in_new_thread.h"
#include "hf_non_copy.h"

namespace hf
{

// 事件循环线程池，暂时不支持线程的动态增加和减少
class EventLoopThreadPool : public NonCopy
{
    public:
        EventLoopThreadPool(EventLoop* base_loop, int num_threads_);
        ~EventLoopThreadPool();

        EventLoop* GetNextEventLoop();
    private:
        // 主事件循环对象
        EventLoop* base_loop_;

        // 表示线程池中线程的个数
        int num_threads_;

        // 使用轮询的方式获取线程池中的线程
        int next_thread_index_;

        // 用来存储线程类
        // 使用智能指针管理EventLoopInNewThread可以不用手动释放内存
        std::vector<std::unique_ptr<EventLoopInNewThread>> thread_pools_;
        // 用来存储每一个线程类相对应的EventLoop指针
        std::vector<EventLoop*> event_loop_pools_;

        // 创建并启动线程池中每一个线程
        // 如果将来需要，应该设置一个启动线程池的开关
        void Start();
};


}
#endif