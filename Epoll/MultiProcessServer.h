//
// Created by cyh on 2021/2/25.
//

#ifndef CHATROOMSERVER_MULTIPROCESSSERVER_H
#define CHATROOMSERVER_MULTIPROCESSSERVER_H

#include <string> //string
#include <unordered_map> //unordered_map
#include <netinet/in.h> //sockaddr_in

#define MAX_EVENT_NUMBER 5000   //Epoll最大事件数
#define FD_LIMIT         10     //最大客户端数量
#define BUFFER_SIZE      1024   //缓存区数据大小

//客户端数据
struct client_data {
    sockaddr_in address;
    int sockfd;
    pid_t pid;
    int pipefd[2];
};

class MultiProcessServer {
public:
    explicit MultiProcessServer();
    ~MultiProcessServer();
    bool InitServer(const std::string &Ip, const int &Port);
    void Run();

private:
    int m_socketFd = 0; //创建的socket文件描述符
    static int s_epollFd; //创建的epoll文件描述符
    static int s_userCount; //客户端编号
    static int s_pipeFd[2]; //信号通信管道
    client_data* m_user;
    std::unordered_map<int, int> m_subProcess; //子进程Pid和客户端编号的映射表
    int m_shmfd; //共享内存标识符
    std::string m_shmName = "/myshm"; //共享内存名称
    char *m_shareMem; //共享内存首地址
    bool m_serverStop = true;
    bool m_terminate = false;
    static bool s_childStop;

private:
    int SetNonblocking(int fd); //将文件描述符设置为非堵塞的
    void Addfd(int epoll_fd, int sock_fd); //将文件描述符上的事件注册
    static void SignalHandler(int sig); //父进程信号处理回调函数
    static void ChildSignalHandler(int sig); //子进程信号处理回调函数
    static void AddSignal(int sig, void(*handler)(int), bool restart = true); //设置信号处理回调函数
    void HandleSignal(const std::string &sigMsg); //自定义信号处理函数
    void RunChild(int idx, client_data* users, char *share_mem); //子进程函数
};

#endif //CHATROOMSERVER_MULTIPROCESSSERVER_H
