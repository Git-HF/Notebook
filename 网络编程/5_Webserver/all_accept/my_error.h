#ifndef MY_ERROR_H
#define MY_ERROR_H

//输出错误信息，并结束进程
inline void my_error_quit(const char* str)
{

    printf("%s: %s\n", str, strerror(errno));
    exit(-1);
}

#endif