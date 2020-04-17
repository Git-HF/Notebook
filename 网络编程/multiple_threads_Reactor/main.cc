#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/timerfd.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>

#include <cstdio>
#include <cstring>

#include "hf_event_loop_in_new_thread.h"
#include "hf_utils.h"
#include "hf_tcp_server.h"
#include "hf_buffer.h"

using namespace hf;

void OnMessage(const TcpConnectionPtr& conn, Buffer* buffer)
{
    if(buffer->ReadableBytes() == 61)
    {
        std::string str = std::move(buffer->RetrieveAllAsString());
        conn->SendData(str);
        conn->ShutDown();
    }
}

int main()
{
    EventLoop loop;
    TcpServer server(&loop, 9876, 4);

    server.SetMessageCallBack(OnMessage);
    server.StartServer();

    loop.Loop();

    return 0;
}