## C++空类中，编译器会为空类提供哪些默认成员函数？
> * 编译器会在需要的时候为一个空类提供6个成员函数：默认构造函数、拷贝构造函数、拷贝赋值运算符、析构函数、取地址符和const取地址符，并且这些函数默认都是内联的。
> * 由于空类是可以被实例化的，是需要占内存的，所以空类所占的内存不是0；为了达到这个目的，编译器往往会给一个空类隐含的加一个字节，这样空类在实例化后再内存中得到独一无二的地址。

## C++虚函数的默认参数
> * 当使用多态调用一个类型中定义的虚函数时，编译器会根据指针的静态类型来选择虚函数的默认参数，而不是动态类型来选择默认参数。所以对于虚函数最好不要定义默认参数；如果一定要使用，派生类的虚函数的默认参数一定要与基类保持一致。

## C++内存管理
> * **堆**：需要自己分配和释放，分配效率较低，大小无限，会产生碎片
> * **栈**：由编译器自动管理，分配效率较高，大小固定，不会产生碎片
> * **静态存储区**：用来存储局部静态变量和全局变量
> * **常量区**：用来存储一些字符串常量。