# Socket聊天小工具

## 开发环境：Windows10

## 开发工具：Visual Studio 2019

## 思路与设计理念：

### 最基本的Socket模型

想要客户端和服务端能在网络中通信，必须得使用Socket编程，它是进程间通信比较特别的方式，而别指出在于它是可以跨主机通信。

在创建Socket的时候，可以指定网络层使用的是IPv6还是IPv4，传输层使用的是TCP还是UDP。本文使用的是TCP编程。

服务端的Socket编程

1. 在开始之前，先初始化socket库，说明要使用的版本。

2. 服务端先调用socket()函数，网络协议IPv6，传输协议TCP。

3. 将服务器的一些信息打包在一个结构体里面，包括地址、协议簇、端口号（设置为52000）。

4. 调用bind()函数，给这个Socket绑定一个IP地址和端口。

5. 调用listen()函数进行监听，此时这个socket用于监听客户端的请求。

6. 服务端进入监听状态后，通过调用accept()函数，来从内核获取客户端的连接，如果没有客户端连接，则会阻塞等待客户端的到来。

服务端的Socket编程

1. 和服务端前三步一致。

2. 调用connect()函数发起连接，该函数的参数要指明服务端的IP地址和端口号，然后TCP就开始进行三次握手了。

3. 当TCP全连接队列不为空后，服务端的accept()函数，就会从内核中的TCP全连接队列拿出一个已完成连接的Socket返回程序，后续数据传输都用这个Socket。此时，这个Socket用作传数据，与监听的Socket不是同一个。

4. 连接建立后，客户端和服务端就可以互相传数据了。

至此，TCP协议的Socket程序的调用过程就结束了，整个过程如下图：



![img](https://github.com/JehanRio/Socket_ChatGroup/blob/main/img/clip_image002.jpg)
### 如何服务更多的用户？

这是我的初版Socket编程，虽然能够成功实现，但整个过程出现了一个巨大的问题，就是它智能一对一通信，而且因为使用的是同步阻塞的方式，当服务端在还没处理完一个客户端的网络I/O时，或者在读写操作发生阻塞时，其他客户端时没法与服务端连接的。而且，由于程序时一条一条执行下去的，当一方发送数据的时候，另一方如果不发送信息则会阻塞在那，直到另一端也发信息，才能接收到对方的信息。

![img](https://github.com/JehanRio/Socket_ChatGroup/blob/main/img/clip_image004.jpg)

### 多线程模式

考虑到进程间上下文切换的“包袱”很重，那我就搞一个比较轻量级的模型来应对用户的请求——多线程模式。

由于同进程的线程可以共享进程的部分资源，比如文件描述符列表、进程空间、代码、全局数据、堆、共享库等，这些共享资源在上下文切换时不需要切换，而只需要切换线程的私有数据、寄存器等不共享的数据，因此同一个进程下的线程上下文切换的开销比进程小得多。

![img](https://github.com/JehanRio/Socket_ChatGroup/blob/main/img/clip_image006.jpg)同时，我还可以使用线程池的方式来避免线程的频繁创建和销毁，将已经连接的Socket放入一个队列，然后线程池的线程负责从队列取出已连接的Socket进程处理。

同时，给服务器Socket绑定一个事件对象，用来接收客户端连接的请求，同时将服务端加入线程池中的第一个元素。将事件与Socket关联在一起后，便可以注册自己感兴趣的网络事件类型。

当服务器与TCP完成连接后，便可以通过WSACreateEvent()函数创建事件。

**服务端：**

在写服务端线程的时候，我采用了并发的思想，运用时间片轮转调度算法使得每个事件都能公平交替轮询到，不会出现饿死现象。

若判断有事件，则用WSAEnumNetworkEvents()函数观看是什么事件：

1. 若产生accept()事件，并且连接数没有超过事件池大小，则分配一个新的Socket和事件给该客户端，并给所有客户端发送欢迎的消息；

2. 若接收到消息，则通过recv()函数接收，若判断为”send a picture”，则创建一个FILE类型的指针接受图片；若不是则将消息广播给所有客户端。

3. 若检测到客户端被关闭，则关闭该事件，并用数组调整，用顺序表删除元素（Socket、事件、地址）并广播给所有客户端退出的消息。

> 此外，再创建一个线程用于计时，调用time.h中的time_t类别，运用difftime()函数，每过20s清一次屏。

**客户端：**

在主函数里面，通过cin.getline()函数阻塞，判断如果等于”quit”则退出聊天室；如果等于“send a picture”则发送图片。

创建一个接收消息的线程，通过recv()函数阻塞在该线程中，如果接收到消息，则将消息打印出来。若recv()函数返回值为负数，则说明该客户端已断开连接。

再创建一个线程用于技术清屏，原理和服务端的一样。

**接收图片**

![img](https://github.com/JehanRio/Socket_ChatGroup/blob/main/img/clip_image008.jpg)

![img](https://github.com/JehanRio/Socket_ChatGroup/blob/main/img/clip_image010.jpg)

**创建10个客户端**

![img](https://github.com/JehanRio/Socket_ChatGroup/blob/main/img/clip_image012.jpg)

**客户端退出**

![img](https://github.com/JehanRio/Socket_ChatGroup/blob/main/img/clip_image014.jpg)

**客户端注销**

![img](https://github.com/JehanRio/Socket_ChatGroup/blob/main/img/clip_image016.jpg)

