#ifndef HF_UTILS_H_
#define HF_UTILS_H_

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace hf
{
    // 输出错误信息的同时退出进程
    inline void ErrorAndQuit(const char* str)
    {
        printf("%s: %s\n", str, strerror(errno));
        exit(0);
    }

    // 只单纯输出str并退出
    inline void OutputAndQuit(const char* str)
    {
        printf("%s\n", str);
        exit(0);
    }

    // 相当于prinf
    inline void Output(const char* str)
    {
        printf("%s\n", str);
    }

    inline void OutputError(const char* str)
    {
        printf("%s: %s\n", str, strerror(errno));
    }
}

#endif