//
// Created by cyh on 2021/2/25.
//

#include <iostream>
#include <fcntl.h> //fcntl()
#include <sys/epoll.h> //epoll
#include <netinet/in.h> //sockaddr_in
#include <arpa/inet.h> //inet_pton()
#include <string.h> //memset()
#include <unistd.h> //close() alarm() ftruncate() fork()
#include <signal.h> //signal
#include <sys/wait.h> //waitpid()
#include <sys/mman.h> //shm_open()
#include "MultiProcessServer.h"

using namespace std;

int MultiProcessServer::s_pipeFd[2];
int MultiProcessServer::s_epollFd = 0;
int MultiProcessServer::s_userCount = 0;
bool MultiProcessServer::s_childStop = false;

MultiProcessServer::MultiProcessServer() = default;

MultiProcessServer::~MultiProcessServer() {
    m_subProcess.clear();
    delete [] m_user;
}

bool MultiProcessServer::InitServer(const std::string &Ip, const int &Port) {
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

    s_epollFd = epoll_create(5);
    if (s_epollFd == -1) {
        cout << "Server: create epoll error!" << endl;
        return false;
    }
    Addfd(s_epollFd, m_socketFd); //注册sock_fd上的事件

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, s_pipeFd); //使用socketpair创建管道，并注册s_pipeFd[0]上可读写事件
    if (ret == -1) {
        cout << "Server: socketpair error!" << endl;
        return false;
    }
    SetNonblocking(s_pipeFd[1]);
    Addfd(s_epollFd, s_pipeFd[0]);

    AddSignal(SIGCHLD, SignalHandler); //子进程状态发生变化
    AddSignal(SIGTERM, SignalHandler); //终止进程，即kill
    AddSignal(SIGINT, SignalHandler); //键盘输入以中断进程 Ctrl+C
    AddSignal(SIGPIPE, SIG_IGN); //忽略管道引起的信号
    m_serverStop = false;
    m_user = new client_data[FD_LIMIT + 1];

    m_shmfd = shm_open(m_shmName.c_str(), O_CREAT | O_RDWR, 0666);
    if (m_shmfd == -1) {
        cout << "Server: open shared memory error!" << endl;
        return false;
    }
    ret = ftruncate(m_shmfd, FD_LIMIT * BUFFER_SIZE);
    if (ret == -1) {
        cout << "Server: create shared memory error!" << endl;
        return false;
    }
    m_shareMem = reinterpret_cast<char *>(mmap(nullptr, FD_LIMIT * BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, m_shmfd, 0));
    if (m_shareMem == MAP_FAILED) {
        cout << "Server: bind shared memory error!" << endl;
        return false;
    }
    close(m_shmfd);

    cout << "INIT SERVER SUCCESS!" << endl;
    return true;
}

void MultiProcessServer::Run() {
    cout << "START SERVER!" << endl;
    int ret;
    epoll_event events[MAX_EVENT_NUMBER];
    while (!m_serverStop) {
        int number = epoll_wait(s_epollFd, events, MAX_EVENT_NUMBER, -1); //epoll_wait
        if (number < 0 && errno != EINTR) {
            cout << "Server: epoll error" << endl;
            break;
        }

        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;
            if (sockfd == m_socketFd) { //新的socket连接
                struct sockaddr_in client_addr;
                socklen_t len = sizeof(client_addr);
                int conn_fd = accept(m_socketFd, (struct sockaddr*)&client_addr, &len);

                if (conn_fd < 0) continue;
                if (s_userCount >= FD_LIMIT) {
                    close(conn_fd);
                    continue;
                }

                m_user[s_userCount].address = client_addr;
                m_user[s_userCount].sockfd = conn_fd;

                ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_user[s_userCount].pipefd);
                if (ret == -1) break;
                pid_t pid = fork();
                if (pid < 0) { //fork 失败
                    close(conn_fd);
                    continue;
                } else if (pid == 0) { //子进程
                    close(s_epollFd);
                    close(m_socketFd);
                    close(m_user[s_userCount].pipefd[0]);
                    close(s_pipeFd[0]);
                    close(s_pipeFd[1]);

                    RunChild(s_userCount, m_user, m_shareMem);
                    munmap(reinterpret_cast<void *>(m_shareMem), FD_LIMIT * BUFFER_SIZE);
                    exit(0);
                } else { //父进程
                    close(conn_fd);
                    close(m_user[s_userCount].pipefd[1]);
                    Addfd(s_epollFd, m_user[s_userCount].pipefd[0]);

                    m_user[s_userCount].pid = pid;
                    m_subProcess[pid] = s_userCount;
                    s_userCount++;
                    cout << "Server: New connect, Now client number:" << s_userCount << endl;
                }
            } else if ((sockfd == s_pipeFd[0]) && (events[i].events & EPOLLIN)) { //处理信号
                char sigBuf[BUFFER_SIZE];
                ret = recv(s_pipeFd[0], sigBuf, sizeof(sigBuf), 0);
                if (ret <= 0) continue;
                else {
                    string sigMsg(sigBuf);
                    HandleSignal(sigMsg); //自定义处理信号
                }
            } else if (events[i].events & EPOLLIN) { //某个子进程向主进程写入数据
                int child = 0;
                ret = recv(sockfd, reinterpret_cast<char *>(&child), sizeof(child), 0);
                if (ret <= 0) continue;
                else {
                    for (int i = 0; i < s_userCount; ++i) {
                        if (m_user[i].pipefd[0] != sockfd){
                            send(m_user[i].pipefd[0], reinterpret_cast<char *>(&child), sizeof(child), 0);
                        }
                    }
                }
            } else {
                cout << "Server: socket something else happened!" << endl;
            }
        }
    }

    cout << "CLOSE SERVER!" << endl;
    close(m_socketFd);
    close(s_epollFd);
    close(s_pipeFd[0]);
    close(s_pipeFd[1]);
    shm_unlink(m_shmName.c_str());//关闭POSIX共享内存
}

int MultiProcessServer::SetNonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void MultiProcessServer::Addfd(int epoll_fd, int sock_fd) {
    epoll_event event;
    event.data.fd = sock_fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &event);
    SetNonblocking(sock_fd);
}

void MultiProcessServer::SignalHandler(int sig) {
    // 保留原有的errno，函数执行后恢复，以保证函数的可重入性
    int old_errno = errno;
    int msg = sig;
    send(s_pipeFd[1], reinterpret_cast<char *>(&msg), 1 , 0); //将信号写入管道，通知主循环
    errno = old_errno;
}

void MultiProcessServer::ChildSignalHandler(int sig) {
    s_childStop = true; //终止子进程主循环
}

void MultiProcessServer::AddSignal(int sig, void(*handler)(int), bool restart) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart) sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    if (sigaction(sig, &sa, nullptr) == -1) {
        cout << "Server: Add signal error!" << endl;
    }
}

void MultiProcessServer::HandleSignal(const string &sigMsg) {
    for (auto &item : sigMsg){ //逐个处理接收的信号
        switch (item) {
            case SIGCHLD: { //子进程退出
                cout << " SIGCHLD: The child process state changes" << endl;
                pid_t pid;
                int stat;
                while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
                    int del_user = m_subProcess[pid];
                    if (del_user < 0 || del_user > FD_LIMIT) continue;
                    epoll_ctl(s_epollFd, EPOLL_CTL_DEL, m_user[del_user].pipefd[0], 0);
                    close(m_user[del_user].pipefd[0]);
                    s_userCount--;
                }
                if (m_terminate && s_userCount == 0) m_serverStop = true; //安全的终止服务器主循环
                break;
            }
            case SIGTERM: {
                cout << " SIGTERM: Kill" << endl;
                m_serverStop = true; //安全的终止服务器主循环
                break;
            }
            case SIGINT: {
                cout << " SIGINT: Ctrl+C" << endl;
                if (s_userCount == 0) {
                    m_serverStop = true; //安全的终止服务器主循环
                    break;
                }
                for (int i = 0; i < s_userCount; ++i) {
                    int pid = m_user[i].pid;
                    kill(pid, SIGTERM);
                }
                m_terminate = true;
                break;
            }
        }
    }
}

void MultiProcessServer::RunChild(int idx, client_data *users, char *share_mem) {
    epoll_event events[MAX_EVENT_NUMBER];
    int child_epollfd = epoll_create(5);
    if (child_epollfd == -1) {
        cout << "Child Process: create epoll error!" << endl;
        exit(0);
    }
    
    int sockfd = users[idx].sockfd;
    Addfd(child_epollfd, sockfd);
    int pipefd = users[idx].pipefd[1];
    Addfd(child_epollfd, pipefd);
    int ret;
    AddSignal(SIGTERM, ChildSignalHandler, false);

    while (!s_childStop) {
        int number = epoll_wait(child_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR) {
            cout << "Child Process: epoll error" << endl;
            break;
        }

        for (int i = 0; i < number; ++i) {
            int fd = events[i].data.fd;
            if ((fd == sockfd) && (events[i].events & EPOLLIN)) { //客户端有数据到达
                memset(share_mem + idx * BUFFER_SIZE, '\0', BUFFER_SIZE);
                ret = recv(fd, share_mem + idx * BUFFER_SIZE, BUFFER_SIZE - 1, 0);
                if (ret < 0) {
                    if (errno != EAGAIN) s_childStop = true; //对端异常断开连接socket
                } else if (ret == 0) {  //对端正常关闭socket
                    s_childStop = true;
                } else {
                    send(pipefd, reinterpret_cast<char *>(&idx), sizeof(idx), 0);
                }
            } else if ((fd == pipefd) && (events[i].events & EPOLLIN)) { //主进程通知本进程，将第client个客户端数据发送给本进程负责的客户端
                int client = 0;
                ret = recv(fd, reinterpret_cast<char *>(&client), sizeof(client), 0);
                if (ret < 0) {
                    if (errno != EAGAIN) s_childStop = true; //对端异常断开连接socket
                } else if (ret == 0) {  //对端正常关闭socket
                    s_childStop = true;
                } else {
                    send(sockfd, share_mem + client * BUFFER_SIZE, BUFFER_SIZE, 0);
                }
            } else {
                cout << "Child Process: something else happened" << endl;
                continue;
            }
        }
    }

    close(sockfd);
    close(pipefd);
    close(child_epollfd);
}
