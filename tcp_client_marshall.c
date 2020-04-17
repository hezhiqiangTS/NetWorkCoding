#include "lib/common.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    error(1, 0, "usage: tcpclient <IP Address>");
  }
}