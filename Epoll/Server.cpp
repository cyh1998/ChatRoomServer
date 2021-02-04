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
#include <signal.h> //signal
#include "Server.h"

using namespace std;

int Server::s_pipeFd[2];

Server::Server()
{
}

bool Server::InitServer(const std::string &Ip, const int &Port)
{
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
    Addfd(m_epollFd, m_socketFd); //注册sock_fd上的事件

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, s_pipeFd); //使用socketpair创建管道，并注册s_pipeFd[0]上可读写事件
    if (ret == -1) {
        cout << "Server: socketpair error!" << endl;
        return false;
    }
    SetNonblocking(s_pipeFd[1]);
    Addfd(m_epollFd, s_pipeFd[0]);

    AddSignal(SIGHUP); //控制终端挂起
    AddSignal(SIGCHLD); //子进程状态发生变化
    AddSignal(SIGTERM); //终止进程，即kill
    AddSignal(SIGINT); //键盘输入以中断进程 Ctrl+C
    m_serverStop = false;
    cout << "INIT SERVER SUCCESS!" << endl;

    return true;
}

int Server::SetNonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void Server::Addfd(int epoll_fd, int sock_fd)
{
    epoll_event event;
    event.data.fd = sock_fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &event);
    SetNonblocking(sock_fd);
}

void Server::BroadcastMsg(const int &fd, const string &msg)
{
    for (auto &i : m_clientsList) {
        if (i != fd) {
            if (send(i, msg.data(), msg.size(), 0) < 0) {
                close(fd);
                m_clientsList.remove(fd);
                cout << "Server: send error! Close client: " << i << endl;
            }
        }
    }
}

void Server::SignalHandler(int sig)
{
    // 保留原有的errno，函数执行后恢复，以保证函数的可重入性
    int old_errno = errno;
    int msg = sig;
    send(s_pipeFd[1], reinterpret_cast<char *>(&msg), 1 , 0); //将信号写入管道，通知主循环
    errno = old_errno;
}

void Server::AddSignal(int sig)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SignalHandler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    if (sigaction(sig, &sa, nullptr) == -1) {
        cout << "Server: Add signal error!" << endl;
    }
}

void Server::HandleSignal(const string &sigMsg) {
    for (auto &item : sigMsg){ //逐个处理接收的信号
        switch (item) {
            case SIGHUP: {
                cout << " SIGHUP: Terminal Hangup" << endl;
            }
            case SIGCHLD: {
                cout << " SIGCHLD: The child process state changes" << endl;
            }
            case SIGTERM: {
                cout << " SIGTERM: Kill" << endl;
            }
            case SIGINT: {
                cout << " SIGINT: Ctrl+C" << endl;
                m_serverStop = true; //安全的终止服务器主循环
            }
        }
    }
}

void Server::Run()
{
    cout << "START SERVER!" << endl;
    epoll_event events[MAX_EVENT_NUMBER];
    while (!m_serverStop) {
        int ret = epoll_wait(m_epollFd, events, MAX_EVENT_NUMBER, -1); //epoll_wait
        if (ret < 0 && errno != EINTR) {
            cout << "Server: epoll error" << endl;
            break;
        }

        for (int i = 0; i < ret; ++i) {
            int sockfd = events[i].data.fd;
            if (sockfd == m_socketFd) { //新的socket连接
                struct sockaddr_in client_addr;
                socklen_t len = sizeof(client_addr);

                int conn_fd = accept(m_socketFd, (struct sockaddr*)&client_addr, &len);
                Addfd(m_epollFd, conn_fd); //对 conn_fd 开启ET模式
                m_clientsList.emplace_back(conn_fd);

                cout << "Server: New connect fd:" << conn_fd << " Now client number:" << m_clientsList.size() << endl;
            } else if ((sockfd == s_pipeFd[0]) && (events[i].events & EPOLLIN)) { //处理信号
                char sigBuf[BUFFER_SIZE];
                ret = recv(s_pipeFd[0], sigBuf, sizeof(sigBuf), 0);
                if (ret <= 0) continue;
                else {
                    string sigMsg(sigBuf);
                    HandleSignal(sigMsg); //自定义处理信号
                }
            } else if (events[i].events & EPOLLIN) { //可读事件
                m_recvStr.clear();
                for (;;) {
                    char client_buf[BUFFER_SIZE];
                    memset(&client_buf, '\0', BUFFER_SIZE);
                    int recvRet = recv(sockfd, client_buf, BUFFER_SIZE - 1, 0);
                    if (recvRet == 0) { //对端正常关闭socket
                        close(sockfd);
                        m_clientsList.remove(sockfd);
                        cout << "Server: Client close socket!" << endl;
                        cout << "Server: Now client number:" << m_clientsList.size() << endl;
                        break;
                    } else if (recvRet < 0){
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) { //读完全部数据
                            BroadcastMsg(sockfd, m_recvStr);
                            break;
                        }
                        if ((errno != EAGAIN) && (errno != EINTR)) { //对端异常断开socket
                            close(sockfd);
                            m_clientsList.remove(sockfd);
                            cout << "Server: Client abnormal close socket!" << endl;
                            cout << "Server: Now client number:" << m_clientsList.size() << endl;
                            break;
                        }
                    } else { //读到一次数据
                        cout << "Server: Recv data: " << client_buf << endl;
                        m_recvStr.append(client_buf);
                    }
                }
            } else {
                cout << "Server: socket something else happened!" << endl;
            }
        }
    }

    cout << "CLOSE SERVER!" << endl;
    close(m_socketFd);
    close(m_epollFd);
    close(s_pipeFd[0]);
    close(s_pipeFd[1]);
}




