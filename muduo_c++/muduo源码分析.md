第一次看muduo源码，记录一下自己的理解
- - - 
### 1. `EventLoop`
- `EventLoop`是程序循环的核心类，其中包括很多其它类，例如`Poller`对象、`Channel`数组、`TimerQueue`对象。每一个`EventLoop`只属于一个线程，其有一个数据成员用来保存自己属于的那个线程。

### 2. `Poller`
- 每一个`EventLoop`都有一个`Poller`对象，`Poller`对象的生命期和一个`EventLoop`一样长。`Poller`是对`poll`函数的封装，实现多路复用的功能。`Poller`只实现多路复用的功能，对于分发功能，是`EventLoop`通过回调函数实现的。当`EventLoop`调用`poll`函数时，得到一个已经准备好的`Channel`的数组；如果数组不为空，则依次调用`Channel`对象的`handleEvent`成员来处理相应的时间；如果数组为空，则继续轮询。

### 3. `Channel`
- `Channel`可以理解成对文件描述符的封装；`Channel`主要用来记录该文件描述符所关注的事件，同时注册回调函数，在相应事件发生时，由`EventLoop`来调用回调函数，完成分发功能；

### 4. `TimerQueue`
- `TimerQueue`表示定时器队列。每一个`EventLoop`都有一个定时器队列成员，用于记录定时器，完成相应的定时功能。`TimerQueue`使用`timer_fd`系列函数，利用文件描述符的可读性来通知时间到达，所以可以和`select、poll`和`epoll`配合使用。`TimerQueue`中的文件描述符`timerfd_`一直被监听，不论是否真的有定时器。
- `TimerQueue`初始化流程：
  * 首先初始化创建自己的`EventLoop`的对象
  * 调用`timer_fd`系列函数来初始化`timerfd_`
  * 将上一步初始化的`timerfd_`用`Channel`来封装一下
  * 创建一个空的`Entry`集合
- `TimerQueue`中只有一个`timer_fd`,表示定时时间最短的那个定时器；每当一个定时器到时间时，会将`timer_fd`设置为下一个定时时间最短的定时器。
- `TimerQueue`中用来存储定时器队列是`set`类型，它是一种搜索二叉树的结构；其关键字是`std::pair(Timestamp, Timer*)`。`std::pair`是可以比较大小的，先比较`first`成员，在比较`second`成员。比较`second`成员就是比较两个指针，虽然比较两个没有关联的指针没有意义，但在这里主要是为了区分定时时间相同的两个定时器。 ***（为啥不直接把定时器的指针作为关键字？？？）***

### 5. `Timestamp`
- 表示时间的一个类，主要就是对时间提供了一个封装，表示从元年开始到现在的时间，其中用`int64_t`来存储从元年到现在经历了多少微秒。
- 静态成员方法`static Timestamp now()`返回当前的系统时间，主要是对`gettimeofday`的封装；
- 对于这个类，未提供一个有两个形参的构造函数`Timestamp(Timestamp, double)`；见文件 ***s09/EventLoop.cc/line89***

### 6. `Timer`
- 这是一个表示定时器的类，其数据成员有：回调函数`callback_`用来表示时间到达时应该发生的事件；`expiration_`表示定时的时间，用`Timestamp`类来表示；`interval_`表示周期性定时的延时时间，用`double`数据类型来表示。如果`interval_`大于0，则表示开启周期性定时，每个`interval_`秒时间到达；如果小于等于0，则表示不需要周期性定时；

### 7. `EventLoopThread`
- 这个类主要数据成员包含一个线程类`Thread`和事件循环类`EventLoop`，其主要作用就是开启一个新的线程，并且在新的线程中创建一个`EventLoop`对象。该类体现了 ***one_loop_per_thread*** 思想。
- `startLoop()`是其关键的成员函数，函数流程为先启动线程，然后在新线程中创建`EventLoop`对象；由于原线程需要返回这个对象的地址，所以需要使用条件变量，让原线程等待新线程创建完`EventLoop`对象才能返回。