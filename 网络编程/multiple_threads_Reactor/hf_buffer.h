#ifndef HF_BUFFER_H_
#define HF_BUFFER_H_

#include <cassert>
#include <vector>
#include <string>
#include <algorithm>

namespace hf
{

// Buffer类是可以拷贝的

class Buffer
{
    private:
        // 缓冲区前面预留八个字节空闲，以备不时之需
        static const std::size_t kCheapPrepend = 8;
        // 缓冲区一开始的大小
        static const std::size_t kInitialSize = 1024;
    
    public:
        Buffer();
        ~Buffer();

        size_t ReadableBytes() const
        {
            return write_index_ - read_index_;
        }

        size_t WriteableBytes() const
        {
            return data_.size() - write_index_;
        }

        // read_index_之前有多少空间是可以重新利用的
        size_t BeforeReadIndex() const 
        {
            return read_index_ - kCheapPrepend;
        }

        // 返回第一个可读字节的指针
        const char* FirstReadablePos() const
        {
            return data_.data() + read_index_;
        }

        // 释放len长度的数据
        void Retrieve(size_t len)
        {
            assert(len <= ReadableBytes());
            read_index_ += len;
        }

        // end指向的数据不释放，即end指向释放区域的尾后元素
        void RetrieveUntil(const char* end)
        {
            assert(FirstReadablePos() <= end);
            assert(end <= (data_.data() + write_index_));
            Retrieve(end - FirstReadablePos());
        }

        void RetrieveAll()
        {
            read_index_ = write_index_ = kCheapPrepend;
        }

        // 前面的Retrieved都不返回数据，只有这个才返回
        std::string RetrieveAllAsString()
        {
            std::string str(FirstReadablePos(), ReadableBytes());
            RetrieveAll();
            return str;
        }

        //将字符串str添加缓冲区尾部
        void AppendStr(const char* str, size_t len)
        {
            EnsureWrite(len);
            std::copy(str, str+len, BeginWritePos());
            write_index_ += len;
        }

        void AppendStr(const std::string &str)
        {
            AppendStr(str.c_str(), str.size());
        }

        // 读取文件描述符，读到缓冲区
        // 如果发生错误，则存储在saved_errno中，
        // 返回读取的字节或者错误情况
        ssize_t ReadFd(int fd, int* saved_errno);

    private:
        std::vector<char> data_;
        // 表示可以从哪里开始读，一直读到write_index_
        std::size_t read_index_;
        // 表示可以从哪里可以写，一直写到data_.size()，write_index_指向可读区域的尾后元素
        std::size_t write_index_;

        // 返回缓冲区开始的位置（不包括prepend）
        char* BeginPos() 
        { 
            return &(*data_.begin()); 
        }

        const char* BeginPos() const
        {
            return &(*data_.begin()); 
        }

        char* BeginWritePos()
        {
            return BeginPos() + write_index_;
        }

        const char* BeginWritePos() const 
        {
            // const版本只能调用const成员函数，所以这里调用的时const版本的BeginPos
            return BeginPos() + write_index_;
        }

        // 确保缓冲可以继续写入len字节的数据;如果不够则需要扩容
        void EnsureWrite(size_t len);
};
}
#endif