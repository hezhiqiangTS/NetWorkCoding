#ifndef HZQCOMMON_H
#define HZQCOMMON_H

#include <arpa/inet.h> /* inet(3) functions */
#include <errno.h>
#include <fcntl.h> /* for nonblocking */
#include <netdb.h>
#include <netinet/in.h> /* sockaddr_in{} and other Internet defns */
#include <poll.h>       /* for convenience */
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> /* for convenience */
#include <sys/ioctl.h>
#include <sys/select.h> /* for convenience */
#include <sys/socket.h> /* basic socket definitions */
#include <sys/stat.h>   /* for S_xxx file mode constants */
#include <sys/sysctl.h>
#include <sys/time.h>  /* timeval{} for select() */
#include <sys/types.h> /* basic system data types */
#include <sys/uio.h>   /* for iovec{} and readv/writev */
#include <sys/un.h>    /* for Unix domain sockets */
#include <sys/wait.h>
#include <time.h> /* timespec{} for pselect() */
#include <unistd.h>

size_t readn(int fd, void *buffer, size_t size);
void error(int status, int err, char *fmt, ...);
size_t read_message(int fd, char *buffer, size_t length);
#define SERV_PORT 43211
#define MAXLINE 4096
#define UNIXSTR_PATH "/var/lib/unixstream.sock"
#define LISTENQ 1024
#define BUFFER_SIZE 4096
#endif