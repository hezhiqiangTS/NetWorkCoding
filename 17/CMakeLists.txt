project(reliable_TCP)
cmake_minimum_required(VERSION 3.5)

include_directories(/home/NetWorkCoding/lib)

set(READ_SRC /home/NetWorkCoding/lib/read.c)
set(SERVER_SRC_LIST tcp_server.c)
set(CLIENT_SRC_LIST tcp_client.c)

add_executable(reliabel_server ${SERVER_SRC_LIST} ${READ_SRC})
add_executable(reliabel_client ${CLIENT_SRC_LIST} ${READ_SRC})