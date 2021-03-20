//
// Created by cyh on 2021/2/1.
//

#ifndef EPOLL_ET_SERVER_H
#define EPOLL_ET_SERVER_H

#include <list> //list
#include <string> //string
#include <vector> //vector
// #include "../Timer/TimerList.h" //基于升序链表的定时器
// #include "../Timer/TimerWheel.h" //时间轮
#include "../Timer/TimerHeap.h" //时间堆

#define MAX_EVENT_NUMBER 5000   //Epoll最大事件数
#define FD_LIMIT         65535  //最大客户端数量
// #define BUFFER_SIZE      0xFFFF //缓存区数据大小

class Server {
public:
    explicit Server();
    ~Server();
    bool InitServer(const std::string &Ip, const int &Port);
    void Run();

private:
    int m_socketFd = 0; //创建的socket文件描述符
    static int s_epollFd; //创建的epoll文件描述符
    static std::list<int> s_clientsList; //已连接的客户端socket列表
    std::string m_recvStr; //读取数据缓存区
    static int s_pipeFd[2]; //信号通信管道
    // TimerList m_timerList; //升序定时器链表
    // TimerWheel m_timerWheel; //时间轮定时器
    TimerHeap m_timerHeap; //时间堆定时器
    static bool s_isAlarm; //是否在进行定时任务
    client_data* m_user;
    bool m_timeout = false; //定时器任务标记
    bool m_serverStop = true;

private:
    int SetNonblocking(int fd); //将文件描述符设置为非堵塞的
    void Addfd(int epoll_fd, int sock_fd); //将文件描述符上的事件注册
    void BroadcastMsg(const int &fd, const std::string &msg); //广播客户端消息
    static void SignalHandler(int sig); //信号处理回调函数
    void AddSignal(int sig); //设置信号处理回调函数
    void HandleSignal(const std::string &sigMsg); //自定义信号处理函数
    static void SignalThreadFunc(sigset_t *set); //自定义信号线程处理函数
    void TimerHandler(); //SIGALRM信号处理函数
    static void TimerCallBack(client_data *user_data); //定时器回调函数
};

#endif //EPOLL_ET_SERVER_H
