cmake_minimum_required(VERSION 3.16)
project(ChatRoomServer)

set(CMAKE_CXX_STANDARD 14)

add_executable(ChatRoomServer main.cpp Epoll/Server.cpp Epoll/Server.h Timer/TimerList.cpp Timer/TimerList.h Timer/TimerWheel.cpp Timer/TimerWheel.h Timer/TimerHeap.cpp Timer/TimerHeap.h)
