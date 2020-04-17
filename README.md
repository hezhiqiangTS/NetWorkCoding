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

