#include <error.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "lib/common.h"

int make_socket(uint16_t port) {
  int sock;
  struct sockaddr_in name;

  sock = socket(PF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  name.sin_family = AF_INET;
  name.sin_port = htons(port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  return sock;
}

void read_data(int sockfd) {
  ssize_t n;
  char buf[1024];

  int time = 0;
  for (;;) {
    fprintf(stdout, "block in read\n");
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

  for (;;) {
    clilen = sizeof(cliaddr);
    connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
    read_data(connfd);
    close(connfd);
  }
}