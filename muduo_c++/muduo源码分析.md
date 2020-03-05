第一次看muduo源码，记录一下自己的理解
- - - 
### 1. `EventLoop`
- `EventLoop`是程序循环的核心类，其中包括很多其它类，例如`Poller`对象、`Channel`数组、`TimerQueue`对象。每一个`EventLoop`只属于一个线程，其有一个数据成员用来保存自己属于的那个线程。

### 2. `Poller`
- 每一个`EventLoop`都有一个`Poller`对象，`Poller`对象的生命期和一个`EventLoop`一样长。`Poller`是对`poll`函数的封装，实现多路复用的功能。`Poller`只实现多路复用的功能，对于分发功能，是`EventLoop`通过回调函数实现的。当`EventLoop`调用`poll`函数时，得到一个已经准备好的`Channel`的数组；如果数组不为空，则依次调用`Channel`对象的`handleEvent`成员来处理相应的时间；如果数组为空，则继续轮询。所以`poll`处于一直轮询的状态，并没有阻塞等待；

### 3. `Channel`
- `Channel`可以理解成对文件描述符的封装；`Channel`主要用来记录该文件描述符所关注的事件，同时注册回调函数，在相应事件发生时，由`EventLoop`来调用回调函数，完成分发功能；

### 4. `TimerQueue`
- `TimerQueue`表示定时器队列。每一个`EventLoop`都有一个定时器队列成员，用于记录定时器，完成相应的定时功能。`TimerQueue`使用`timer_fd`系列函数，利用文件描述符的可读性来通知时间到达，所以可以和`select、poll`和`epoll`配合使用。
- `TimerQueue`初始化流程：
  * 首先初始化创建自己的`EventLoop`的对象
  * 调用`timer_fd`系列函数来初始化`timerfd_`
  * 将上一步初始化的`timerfd_`用`Channel`来封装一下
  * 创建一个空的`Entry`集合

### 5. `Timestamp`
- 表示时间的一个类，主要就是对时间提供了一个封装，表示从元年开始到现在的时间，其中用`int64_t`来存储从元年到现在经历了多少微秒。
- 静态成员方法`static Timestamp now()`返回当前的系统时间，主要是对`gettimeofday`的封装；
- 对于这个类，未提供一个有两个形参的构造函数`Timestamp(Timestamp, double)`；见文件 ***s09/EventLoop.cc/line89***

### 6. 