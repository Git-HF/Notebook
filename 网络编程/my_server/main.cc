#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/timerfd.h>
#include <pthread.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>

#include "hf_event_loop.h"
#include "hf_utils.h"
#include "hf_mutex.h"

using namespace hf;

int i = 0;

void run1()
{
    printf("run %d\n", CurrentThread::GetID());
}

void thread_func(void)
{
    printf("thead_fun\n");
    EventLoop loop;
    loop.Loop();

}
int main()
{
    for(int i = 0; i < 10; ++i)
    {
        Thread t(thread_func);
        t.StartThread();
    }

    pause();
    
    return 0;

}