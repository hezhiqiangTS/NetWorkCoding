project(GRACE)
cmake_minimum_required(VERSION 3.5)


include_directories(/home/NetWorkCoding/lib)


set(READ_SRC /home/NetWorkCoding/lib/read.c)
set(SERVER_SRC_LIST graceserver.c)
set(CLIENT_SRC_LIST graceclient.c)

add_executable(graceserver ${SERVER_SRC_LIST} ${READ_SRC})
add_executable(graceclient ${CLIENT_SRC_LIST} ${READ_SRC})