#include "common.h"

// 读取报文预设大小字节
size_t readn(int fd, void *buffer, size_t size) {
  char *buffer_pointer = buffer;
  int length = size;

  while (length > 0) {
    int result = read(fd, buffer_pointer, length);
    if (result < 0) {
      if (errno == EINTR)
        continue;
      else
        return -1;

    } else if (result == 0)
      break;

    length -= result;
    buffer_pointer += result;
  }
  return (size - length);
}

// 读取消息
size_t read_message(int fd, char *buffer, size_t length) {
  u_int32_t msg_length;
  u_int32_t msg_type;
  int rc;

  rc = readn(fd, (void *)&buffer, sizeof(u_int32_t));
  if (rc != sizeof(u_int32_t)) return rc < 0 ? -1 : 0;
  msg_length = ntohl(msg_length);

  rc = readn(fd, (void *)&msg_type, sizeof(u_int32_t));
  if (rc != sizeof(u_int32_t)) return rc < 0 ? -1 : 0;

  if (msg_length > length) {
    return -1;
  }

  rc = readn(fd, buffer, msg_length);
  if (rc != msg_length) return rc < 0 ? -1 : 0;

  return rc;
}