//
// Created by cyh on 2021/2/1.
//
#include <iostream>
#include <fcntl.h> //fcntl()
#include <sys/epoll.h> //epoll
#include <netinet/in.h> //sockaddr_in
#include <arpa/inet.h> //inet_pton()
#include <string.h> //memset()
#include <unistd.h> //close()
#include "Server.h"

using namespace std;

Server::Server() {

}

bool Server::InitServer(const std::string &Ip, const int &Port) {
    int ret;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address)); //初始化 address
    address.sin_family = AF_INET;
    inet_pton(AF_INET, Ip.c_str(), &address.sin_addr);
    address.sin_port = htons(Port);

    m_socketFd = socket(AF_INET, SOCK_STREAM, 0); //创建socket
    if (m_socketFd < 0) {
        cout << "Server: socket error! id:" << m_socketFd << endl;
        return false;
    }

    ret = bind(m_socketFd, (struct sockaddr*)&address, sizeof(address)); //bind
    if (ret == -1) {
        cout << "Server: bind error!" << endl;
        return false;
    }

    ret = listen(m_socketFd, 20); //listen
    if (ret == -1) {
        cout << "Server: listen error!" << endl;
        return false;
    }

    m_epollFd = epoll_create(5);
    if (m_epollFd == -1) {
        cout << "Server: create epoll error!" << endl;
        return false;
    }
    addfd(m_epollFd, m_socketFd, true); //注册sock_fd上的事件

    return true;
}

int Server::setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void Server::addfd(int epoll_fd, int sock_fd, bool epoll_et) {
    epoll_event event;
    event.data.fd = sock_fd;
    event.events = EPOLLIN;
    if (epoll_et) event.events |= EPOLLET;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &event);
    setnonblocking(sock_fd);
}

void Server::Run() {
    epoll_event events[MAX_EVENT_NUMBER];

    while (1) {
        int ret = epoll_wait(m_epollFd, events, MAX_EVENT_NUMBER, -1); //epoll_wait
        if (ret < 0) {
            cout << "Server: epoll error" << endl;
            break;
        }

        char buf[BUFFER_SIZE];
        for (int i = 0; i < ret; ++i) {
            int sockfd = events[i].data.fd;
            if (sockfd == m_socketFd) { //新的socket连接
                struct sockaddr_in client_addr;
                socklen_t len = sizeof(client_addr);
                int conn_fd = accept(m_socketFd, (struct sockaddr*)&client_addr, &len);

                addfd(m_epollFd, conn_fd, true); //对 conn_fd 开启ET模式
                m_clientsList.emplace_back(conn_fd);

                cout << "Server: New connect fd:" << conn_fd << " Now client number:" << m_clientsList.size() << endl;
            } else if (events[i].events & EPOLLIN) { //可读事件
                char client_buf[BUFFER_SIZE];
                memset(&client_buf, '\0', BUFFER_SIZE);
                int recvRet = recv(sockfd, client_buf, BUFFER_SIZE - 1, 0);
                if (recvRet == 0) { //对端正常关闭socket
                    close(sockfd);
                    m_clientsList.remove(sockfd);
                    cout << "Server: Client close socket!" << endl;
                    cout << "Server: Now client number:" << m_clientsList.size() << endl;
                } else if (recvRet < 0){
                    if ((errno != EAGAIN) && (errno != EINTR)) { //对端异常断开socket
                        close(sockfd);
                        m_clientsList.remove(sockfd);
                        cout << "Server: Client abnormal close socket!" << endl;
                        cout << "Server: Now client number:" << m_clientsList.size() << endl;
                    } else { //recv error
                        cout << "Server: Recv error!" << endl;
                    }
                } else {
                    cout << "Server: Recv data: " << client_buf << endl;
                    for (auto &i : m_clientsList) {
                        if (i != sockfd) {
                            if (send(i, client_buf, BUFFER_SIZE, 0) < 0) {
                                close(sockfd);
                                m_clientsList.remove(sockfd);
                                cout << "Server: send error! Close client: " << i << endl;
                            }
                        }
                    }
                }
            } else {
                cout << "Server: socket something else happened!" << endl;
            }
        }
    }

    close(m_socketFd);
    close(m_epollFd);
}




