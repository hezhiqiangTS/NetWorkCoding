#include "../lib/common.h"

int main(int argc, char** argv) {
  int listenfd;
  char buf[1024];

  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in server_addr;
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERV_PORT);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  int on = 1;
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  int rt1 = bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
  if (rt1 < 0) {
    error(1, errno, "bind failed");
  }

  int rt2 = listen(listenfd, LISTENQ);
  if (rt2 < 0) {
    error(1, errno, "listen failed");
  }

  int connfd;
  struct sockaddr_in client_addr;
  socklen_t client_lengh = sizeof(client_addr);

  if ((connfd = accept(listenfd, (struct sockaddr*)&client_addr,
                       &client_lengh)) < 0) {
    error(1, errno, "accept failed");
  }

  // for (;;) {
  int n = read(connfd, buf, 1024);
  if (n < 0) {
    error(1, errno, "error read");
  } else if (n == 0) {
    error(1, 0, "client closed\n");
  }
  fputs(buf, stdout);
  close(connfd);
  sleep(5);
  /*
  int write_nc = send(connfd, buf, n, 0);
  printf("send bytes: %zu \n", write_nc);
  if (write_nc < 0) {
    error(1, errno, "error write");
  }*/
  //}

  exit(0);
}
