#include "hf_buffer.h"

#include <sys/uio.h>

namespace hf
{

#define HELP_BUFFER_SIZE 65536

Buffer::Buffer()
: data_(kCheapPrepend + kInitialSize),
  read_index_(kCheapPrepend),
  write_index_(kCheapPrepend)
{
    assert(ReadableBytes() == 0);
    assert(WriteableBytes() == kInitialSize);
    assert(BeforeReadIndex() == 0);
}

Buffer::~Buffer()
{
    // 是否需要判断缓冲区为空？？？

}
    
void Buffer::EnsureWrite(size_t len)
{
    // 剩余空间足够了，则直接返回
    if(len <= WriteableBytes())
    {
        return;
    }

    // read_index_之前 + write_index_之后的空间足够了
    if((BeforeReadIndex() + WriteableBytes()) >= len)
    {
        assert(kCheapPrepend < read_index_);
        // 记录可读字节，不然在移动后可能有问题
        size_t readable = ReadableBytes();
        std::copy(BeginPos() + read_index_, BeginPos()+write_index_, BeginPos() + kCheapPrepend);

        read_index_ = kCheapPrepend;
        write_index_ = read_index_ + readable;
        assert(readable == ReadableBytes());
    }
    else
    {
        // 可能有一个需要优化的点
        // 需要将read_index_之前的空间清空，移动数据，但是这样好像有点麻烦
        data_.resize(write_index_ + len);
    }

    assert(len <= WriteableBytes());
}

ssize_t Buffer::ReadFd(int fd, int* saved_errno)
{
    // 分配栈上的辅助缓冲区
    char help_buffer[HELP_BUFFER_SIZE];
    iovec vec[2];
    vec[0].iov_base = BeginWritePos();
    vec[0].iov_len = WriteableBytes();
    vec[1].iov_base = help_buffer;
    vec[1].iov_len = HELP_BUFFER_SIZE;

    const size_t writeable = WriteableBytes();
    const ssize_t n = readv(fd, vec, 2);

    // 出错
    if(n < 0)
    {
        *saved_errno = errno;
    }
    else if(static_cast<size_t>(n) <= writeable)
    {
        // 读取的字节数小于缓冲区可容纳的字节数
        write_index_ += n;
    }
    else
    {
        // 读取的字节数超过了缓冲区可容纳的字节数，需要将多出来的自己附加到缓冲区后面
        write_index_ = data_.size();
        AppendStr(help_buffer, n - writeable);
    }

    return n;
}

}