
#include "../lib/common.h"

static int count;

static void sig_int(int signo) {
  printf("\nreceived %d datagrams\n", count);
  exit(0);
}

static void sig_pipe(int signo) {
  printf("\n [SIGPIPE] received %d datagrames\n", count);
  exit(0);
}

int main(int argc, char **argv) {
  int listenfd;
  listenfd = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in server_addr;
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(SERV_PORT);

  int rt1 =
      bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (rt1 < 0) {
    error(1, errno, "bind failed ");
  }

  int rt2 = listen(listenfd, LISTENQ);
  if (rt2 < 0) {
    error(1, errno, "listen failed ");
  }

  signal(SIGINT, sig_int);
  signal(SIGPIPE, sig_pipe);

  int connfd;
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  if ((connfd = accept(listenfd, (struct sockaddr *)&client_addr,
                       &client_len)) < 0) {
    error(1, errno, "bind failed ");
  }

  char message[MAXLINE];
  count = 0;

  for (;;) {
    // 对一个已经收到 FIN 包的连接进行 read 时返回 0 字节
    int n = read(connfd, message, MAXLINE);
    if (n < 0) {
      error(1, errno, "error read");
    } else if (n == 0) {
      error(1, 0, "client closed \n");
    }
    message[n] = 0;
    printf("received %d bytes: %s\n", n, message);
    count++;

    char send_line[MAXLINE];
    sprintf(send_line, "Hi, %s", message);

    sleep(5);

    int write_nc = send(connfd, send_line, strlen(send_line), 0);
    if (write_nc < 0) {
      error(1, errno, "error write");
    }
    // 第二次 write 处于 RST 状态的连接会触发SIG_PIPE
    write_nc = send(connfd, send_line, strlen(send_line), 0);
    printf("send bytes: %zu \n", write_nc);
    if (write_nc < 0) {
      error(1, errno, "error write");
    }
  }
}