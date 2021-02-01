cmake_minimum_required(VERSION 3.16)
project(Epoll_ET)

set(CMAKE_CXX_STANDARD 14)

add_executable(Epoll_ET main.cpp Epoll/Server.cpp Epoll/Server.h)