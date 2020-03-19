关于**webbench**的功能和原理，这里不做介绍，详情可见[webbench介绍](https://www.jianshu.com/p/dc1032b19c8d)。  
***
[下载与安装教程](https://blog.csdn.net/deep_kang/article/details/81204489)
***
虽然webbench是一个web服务器的压测软件，但是它只是发送GET请求，并且对于服务端的响应不要求必须是HTTP响应报文，他对QPS的计算只是计算每次的请求是否有响应，所以webbench也可以用来测试echo服务器。
