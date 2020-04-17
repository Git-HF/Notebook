#ifndef HF_NONCOPY_H_
#define HF_NONCOPY_H_

// 继承自该基类的派生类不可复制和赋值
class NonCopy
{
    private:
        //拷贝构造
        NonCopy(const NonCopy&) = delete;
        //拷贝赋值
        NonCopy& operator=(const NonCopy&) = delete;
    
    protected:
        //默认构造
        NonCopy() = default;
        //析构函数
        ~NonCopy() = default;
};
#endif