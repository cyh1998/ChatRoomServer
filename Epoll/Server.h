//
// Created by cyh on 2021/2/1.
//

#ifndef EPOLL_ET_SERVER_H
#define EPOLL_ET_SERVER_H

#include <list> //list
#include <string>

#define MAX_EVENT_NUMBER 5000   //Epoll最大事件数
#define BUFFER_SIZE      0xFFFF //缓存区数据大小

class Server {
public:
    explicit Server();
    bool InitServer(const std::string &Ip, const int &Port);
    void Run();

private:
    int m_socketFd;    //创建的socket文件描述符
    int m_epollFd;     //创建的epoll文件描述符
    std::list<int> m_clientsList;  //已连接的客户端socket列表
    std::string m_recvStr;      //读取数据缓存区

private:
    int SetNonblocking(int fd); // 将文件描述符设置为非堵塞的
    void Addfd(int epoll_fd, int sock_fd, bool epoll_et); // 将文件描述符上的事件注册
    void BroadcastMsg(const int &fd, const std::string &msg);
};

#endif //EPOLL_ET_SERVER_H
