#include "../lib/common.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    error(1, 0, "usage: reliable_client <IPAddress>");
  }

  int sock_fd;
  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERV_PORT);
  inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

  int rt1 =
      connect(sock_fd, (struct sock_addr*)&server_addr, sizeof(server_addr));

  if (rt1 < 0) {
    error(1, errno, "connect failed");
  }

  char buf[128];
  int len;
  int rc;

  while (fgets(buf, sizeof(buf), stdin) != NULL) {
    len = sizeof(buf);
    rc = send(sock_fd, buf, len, 0);
    if (rc < 0) {
      error(1, errno, "send failed");
    }
    printf("send done\n");
    /*
        rc = read(sock_fd, buf, sizeof(buf));
        if (rc < 0) {
          error(1, errno, "read failed");
        } else if (rc == 0)
          error(1, 0, "peer connection closed\n");
        else
          fputs(buf, stdout);*/
  }
  exit(0);
}