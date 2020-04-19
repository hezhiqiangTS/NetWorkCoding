[TOC]
---
# 网络编程

## 可靠传输工作原理
### 停止等待
发送方在发送一个分组之后就原地停止，等待接受方对之前分组的确认。在接收到前一个分组的确认后再发送下一个分组。当超时未受到确认，则重传。

* 发送方在收到确认之前需要暂时保存刚发送的分组，用于重传
* 分组和确认都需要序列号
* 接受方在收到重复分组之后需要删除重复分组，对于重复分组也需要确认（产生重复分组的原因可能是确认丢失/迟到）

停止等待协议实现简单，但是对于信道的利用率很低。

### 连续ARQ(Automatic Repeat-reQuest)
采用流水线式的发送方式，提高信道利用率。

一次发送发送窗口中的所有分组，收到对n个分组的确认之后，把发送窗口前移n个窗口，继续下一轮发送。

接收方采用累积确认：对按序到达的最后一个分组进行确认。

## TCP如何实现可靠传输
### TCP 报文段的首部格式

### TCP 可靠传输的实现
使用滑动窗口实现。

## 通过send和read来收发数据包
```c
// tcp_server
#include <error.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "lib/common.h"

/*内部循环读取数据，还需要处理EOF*/
void read_data(int sockfd) {
  ssize_t n;
  char buf[1024];

  int time = 0;
  for (;;) {
    fprintf(stdout, "block in read\n");
    // 一次读取1024字节
    if ((n = readn(sockfd, buf, 1024)) == 0) return;

    time++;
    fprintf(stdout, "1K read for %d \n", time);
    usleep(1000);
  }
}

int main(int argc, char **argv) {
  int listenfd, connfd;
  socklen_t clilen;
  struct sockaddr_in cliaddr, servaddr;

  listenfd = socket(AF_INET, SOCK_STREAM, 0);

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(12345);

  bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

  listen(listenfd, 1024);

  /*循环处理不同连接*/
  for (;;) {
    clilen = sizeof(cliaddr);
    connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
    read_data(connfd);
    close(connfd);
  }
}
```
tcp_server 一次从sockfd中读取1024字节
```c
// tcp_client
#include "lib/common.h"

#define MESSAGE_SIZE 10240000

void send_data(int sockfd) {
  char* query;
  query = malloc(MESSAGE_SIZE + 1);
  for (int i = 0; i < MESSAGE_SIZE; i++) {
    query[i] = 'a';
  }
  query[MESSAGE_SIZE] = '\0';

  const char* cp;
  cp = query;
  size_t remaining = strlen(query);
  while (remaining) {
    int n_written = send(sockfd, cp, remaining, 0);
    fprintf(stdout, "send into buffer %ld \n", n_written);
    if (n_written <= 0) {
      error(1, errno, "send failed");
      return;
    }
    remaining -= n_written;
    cp += n_written;
  }
  return;
}

int main(int argc, char** argv) {
  int sockfd;
  struct sockaddr_in servaddr;

  if (argc != 2) error(1, 0, "usage: tcpclient <IPaddress>");

  /*初始化套接字*/
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(12345);
  inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

  /*进行连接*/
  int connect_rt =
      connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
  if (connect_rt < 0) {
    error(1, errno, "connect failed");
  }

  /*发送数据*/
  send_data(sockfd);
  exit(0);
}
```
tcp_client 发送10240000字节数据。

当客户端send_data结束，客户端退出后，可以看到服务端依然还在继续接收数据。
```bash
...
block in read
1K read for 7856 
block in read
1K read for 7857 
block in read
1K read for 7858 
block in read
1K read for 7859 
block in read
1K read for 7860 
block in read
1K read for 7861 
block in read
1K read for 7862 
block in read
1K read for 7863 
block in read
1K read for 7864 
block in read
1K read for 7865 
block in read
1K read for 7866 
block in read
1K read for 7867 
block in read
1K read for 7868 
block in read
1K read for 7869 
...
```
所以需要牢记两点：
* 对于 send 来说，返回成功仅仅表示数据写到发送缓冲区成功，并不表示对端已经成功收到。
* 对于 read 来说，需要循环读取数据，并且需要考虑 EOF 等异常条件

## 关闭连接

主动关闭连接方调用close：
* close本质是对 fd 的引用计数减一
* 引用计数为0，关闭读写——在当前端读写都关闭
* 引用计数减一后不为0（有其他进程引用该fd），不关闭读写

主动关闭连接方调用shutdown：
* 根据传递参数选择性关闭读、写或者读写同时关闭
* shutdown不存在引用计数概念，一定会发出FIN包
* 由于一定发出FIN包，会对其他引用fd的进程产生影响

> 内核处理close和shutdown最关键的区别

在调用close和shutdown后都发送FIN包的情况下，如果连接对端依然有数据包发送过来：
* 若是连接是close关闭，则主动关闭方会返回一个RST
* 若是shutdown关闭单向，则主动关闭方会返回一个ACK

当返回RST时，对端会把连接标记为RST状态，如果对RST状态的连接进行write，则会触发SIG_PIPE。

所以，假如有一方通过close关闭了连接，那么另一方有两种方式检测本链接已经关闭：
1. read(fd) 返回 0
2. write(fd) 触发 SIG_PIPE。

注意，第一次对已关闭连接进行write时，write会正常返回。因为 write 是把用户空间的数据拷贝到socket发送缓冲区中，当第一次调用write时，本端只收到一个FIN，本端默认是可以发送数据的。当数据包达到对方主机网络协议栈之后，对端返回一个RST。当本端接收到RST，将连接标记为RST状态后，再进行write时才会触发SIG_PIPE。

## 端口复用
TCP 连接中 Server 进程退出后立即重启，会因为 TIME_WAIT 机制的作用，导致 Server 无法使用之前的端口，出现 **Address Already in Use** 的错误。

启用重用套接字选项可以解决这种问题。
```c
int on = 1;
setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
```
完整代码如下：
```c
int main(int argc, char **argv) {
    int listenfd;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERV_PORT);

    // 在Server启用重用套接字选项
    // 位置在 bind 之前
    int on = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    int rt1 = bind(listenfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (rt1 < 0) {
        error(1, errno, "bind failed ");
    }

    int rt2 = listen(listenfd, LISTENQ);
    if (rt2 < 0) {
        error(1, errno, "listen failed ");
    }

    signal(SIGPIPE, SIG_IGN);

    int connfd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    if ((connfd = accept(listenfd, (struct sockaddr *) &client_addr, &client_len)) < 0) {
        error(1, errno, "bind failed ");
    }

    char message[MAXLINE];
    count = 0;

    for (;;) {
        int n = read(connfd, message, MAXLINE);
        if (n < 0) {
            error(1, errno, "error read");
        } else if (n == 0) {
            error(1, 0, "client closed \n");
        }
        message[n] = 0;
        printf("received %d bytes: %s\n", n, message);
        count++;
    }
}
```

以上场景只是端口复用的一个例子。实际上端口复用还有其他用处：
> 主机上有多个网卡，即有多个IP地址。为了在不同地址上的相同端口提供服务，可以使用端口复用。

比如，一台服务器有 192.168.1.101 和 10.10.2.102 连个地址，我们可以在这台机器上启动三个不同的 HTTP 服务，第一个以本地通配地址 ANY 和端口 80 启动；第二个以 192.168.101 和端口 80 启动；第三个以 10.10.2.102 和端口 80 启动。

这样目的地址为 192.168.101，目的端口为 80 的连接请求会被发往第二个服务；目的地址为 10.10.2.102，目的端口为 80 的连接请求会被发往第三个服务；目的端口为 80 的所有其他连接请求被发往第一个服务。

我们必须给这三个服务设置 SO_REUSEADDR 套接字选项，否则第二个和第三个服务调用 bind 绑定到 80 端口时会出错。

**事实上，服务端套接字都应该设置 SO_REUSEADDR 选项，以便服务端程序可以在极短的时间内复用同一个端口启动。**

Linux内核是以四元组(客户端地址：端口，服务端地址：端口)来标记一个TCP连接的，因此重用一个端口不会产生任何安全问题。只要客户端使用不同的端口，那么内核就可以识别当前连接是新连接。就算客户端端口不变，内核可以通过序列号和时间戳来识别新连接。

但是，Linux不会允许在**相同的地址和端口上绑定不同的服务端实例**，即使设置了 SO_REUSEADDR 选项。即不可能在 ANY 通配符地址下和端口 9527 上重复启动两个服务端实例。

**可以这么理解 SO_REUSEADDR：告诉内核，如果端口已被占用，但是 TCP 连接状态位于 TIME_WAIT ，可以重用端口。如果连接处于  ESTABLISHED 状态，则不允许重用**

## 理解TCP中的流

在TCP连接的发送端，调用send时，数据并没有被真正从网络上发送出去，只是从应用程序拷贝到了操作系统内核的缓冲区中，何时真正被发送取决于发送窗口、拥塞窗口以及当前发送缓冲区的大小等条件。

也就说是，发送端发送的数据不是以send写入的数据作为基本单位的。假设发送端陆续调用send函数，先后发送network和program报文，那么实际的发送可能会有如下情况：
> 一次性将network和program在一个TCP分组中发送出去
```
...xxxnetworkprogramxxx...
```
两次send调用，实际上只发送了一个TCP数据包
> program的部分随network在一个TCP分组中发送
分组1
```
...xxxxxxnetworkprog
```
分组2
```
ramxxxxxxx....
```
> network的一部分随TCP分组发送，另一部分和program一起发出
分组1
```
...xxxxxnet
```
分组2
```
workprogramxxx.....
```

事实上，上述划分有无数种。核心在于，**我们不知道两次send写入缓冲区的数据是如何进行TCP分组传输的**

在接收端，情况有所不同。接收端缓冲区保留了没有被应用程序取走的数据，随着应用程序不断从缓冲区取出数据，接受缓冲区就可以容纳更多的新数据。如果我们使用recv从接收端缓冲区读取数据，接收端最终接收到的字节流总是像下面这样的：
```
xxxxxxxxnetworkprogramxxxxxx
```
发送的network和program的顺序是严格保持的。如果发送过程中有TCP分组丢失，但是其后续分组陆续到达，那么TCP协议栈会缓存后续分组，直到丢失的分组到达。这两点都是内核中的TCP协议栈严格保证的。


那么问题来了，如果我们就是想要达到network和program处于不同“分组”的状态，服务端一次只响应一个send所传输的数据应该怎么办？

接收端接收的是数据流，无法天然划分对方应用层传输的数据。需要引入报文格式和报文解析来达到这一点。

发送端通过报文格式传输数据，接收端按照预定的接收格式解析数据，这样就可以区分数据流中的数据是哪一个消息中的。

报文格式有两种：
1. 发送端把要发送的报文长度预先用报文告知接收端（显式编码报文长度）；
2. 通过一个特殊的字符来进行边界的划分

## 不可靠的TCP
TCP的可靠传输是从发送端套接字缓冲区到接收端套接字缓冲区之间的可靠传输。

调用send只能保证数据被可靠地发往对端socket的接收缓冲区，无法判断对端的应用层是否已经接收发送的数据流，如果需要这部分信息，那么需要在应用层添加逻辑处理，如显式的报文确认机制。

如果我们需要从应用层感知TCP连接的异常，那么需要在应用层添加异常感知机制。

### 对端无FIN包发送
#### 网络中断造成的对端无FIN包
很多原因会导致网络中断，此时TCP程序无法立即感知到异常信息。

如果TCP程序正好阻塞在read上，那么程序无法从异常中恢复。可以通过给read操作设置超时来解决。

如果程序先write操作把数据放入缓冲区，再阻塞到read调用上，情况会有所不同。由于连接中断，TCP协议栈会不停尝试将缓冲区中的数据发送出去，在尝试大概12次累计约9分钟之后，协议栈会标识该连接异常，阻塞的read调用会返回一个TIMEOUT错误信息。

#### 系统崩溃造成的对端无FIN包
系统突然奔溃，如断电时，网络连接上来不及发出任何东西，导致没有FIN包被发送。在没有ICMP报文的情况下，TCP程序只能通过read和write调用获得网络连接异常的信息。如果对端一直没有重启，那么read和write会得到超时错误。

如果对端重启，由于对端此时没有与本端connect过，也就没有连接的信息，那么就会返回一个RST错误信息。


### 对端有FIN包发送
当对端正常退出前调用close或者shutdown显式关闭连接时会有FIN包发送，也有可能是对端应用崩溃，OS代为发送。**从另一端是无法分辨这两种情况的**。

当接收到FIN报文之后，会在本端的接收缓冲区添加一个EOF标志。当read操作读到EOF时，read调用返回值为0。

假如对端崩溃，FIN是由OS代为发送。且本端不进行read操作，只进行write操作（接收到FIN报文之后进行write也是合法的，因为FIN只表示对端不会再发送数据，不代表不会接收数据），那么当分组到达对端之后，对端OS发现没有对应的进程接收分组，那么就会返回一个RST。本端接收到RST后再次write时，就会触发SIG_PIPE。


