cmake_minimum_required(VERSION 3.16)
project(ChatRoomServer)

set(CMAKE_CXX_STANDARD 14)

#添加 Epoll Timer 子目录
add_subdirectory(Epoll)
add_subdirectory(Timer)

add_executable(ChatRoomServer main.cpp)
add_executable(MultiProcessServer main_two.cpp)

#添加链接库
target_link_libraries(ChatRoomServer Server Timer)
target_link_libraries(MultiProcessServer Server -lrt)
