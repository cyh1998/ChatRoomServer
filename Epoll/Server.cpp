//
// Created by cyh on 2021/2/1.
//
#include <iostream>
#include <fcntl.h> //fcntl()
#include <sys/epoll.h> //epoll
#include <netinet/in.h> //sockaddr_in
#include <arpa/inet.h> //inet_pton()
#include <string.h> //memset()
#include <unistd.h> //close() alarm()
#include <signal.h> //signal
#include <pthread.h>
#include "Server.h"
#include "../Thread/ThreadObject.h"

using namespace std;

int Server::s_pipeFd[2];
int Server::s_epollFd = 0;
std::list<int> Server::s_clientsList;
bool Server::s_isAlarm = false;

Server::Server() = default;

Server::~Server() {
    s_clientsList.clear();
    // m_users.clear();
    delete [] m_user;
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

    // AddSignal(SIGHUP); //控制终端挂起
    // AddSignal(SIGCHLD); //子进程状态发生变化
    AddSignal(SIGTERM); //终止进程，即kill
    AddSignal(SIGINT); //键盘输入以中断进程 Ctrl+C
    AddSignal(SIGALRM); //定时器到期
    
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGQUIT);
    sigaddset(&set, SIGUSR1);
    ret = pthread_sigmask(SIG_BLOCK, &set, nullptr);
    if (ret != 0) {
        cout << "Server: pthread_sigmask error!" << endl;
        return false;
    }
    ThreadObject sigThread([_set = &set] { Server::SignalThreadFunc(_set); });
    sigThread.start();

    m_serverStop = false;
    m_user = new client_data[FD_LIMIT];
    // user = std::vector<client_data>(FD_LIMIT).data();
    // alarm(TIMESLOT); //开始定时

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
    for (auto &i : s_clientsList) {
        if (i != fd) {
            if (send(i, msg.data(), msg.size(), 0) < 0) {
                close(fd);
                s_clientsList.remove(fd);
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

void Server::SignalThreadFunc(sigset_t *set) {
    int ret, sig;
    for (;;) {
        ret = sigwait(set, &sig);
        if (ret == 0) {
            std::cout << "SignalThread: Get signal: " << sig << std::endl;
        }
    }
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
                m_serverStop = true; //安全的终止服务器主循环
            }
            case SIGINT: {
                cout << " SIGINT: Ctrl+C" << endl;
                m_serverStop = true; //安全的终止服务器主循环
            }
            case SIGALRM: {
                // 用timeout来标记有定时任务
                // 先不处理，因为定时任务优先级不高，优先处理其他事件
                m_timeout = true;
                break;
            }
        }
    }
}

void Server::TimerHandler() {
    // m_timerList.Tick(); //调用升序定时器链表类的Tick()函数 处理链表上到期的任务
    // m_timerWheel.Tick(); //调用时间轮的心搏函数 Tick() 处理链表上到期的任务
    // alarm(TIMESLOT);  //再次发出 SIGALRM 信号
    
    m_timerHeap.Tick(); //调用时间堆的心搏函数 Tick() 处理到期的任务
    heap_timer *tmp = nullptr;
    // 判断当前是否在进行定时任务，没有则触发 SIGALRM 信号
    if (!s_isAlarm && (tmp = m_timerHeap.Top())) {
        time_t delay = tmp->expire - time(nullptr);
        if (delay <= 0) delay = 1;
        alarm(delay);
        s_isAlarm = true;
    }
}

void Server::TimerCallBack(client_data *user_data) {
    epoll_ctl(s_epollFd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    if (user_data) {
        close(user_data->sockfd);
        s_clientsList.remove(user_data->sockfd);
        s_isAlarm = false;
        cout << "Server: close socket fd : " << user_data->sockfd << endl;
    }
}

void Server::Run()
{
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
                Addfd(s_epollFd, conn_fd); //对 conn_fd 开启ET模式
                s_clientsList.emplace_back(conn_fd);
                cout << "Server: New connect fd:" << conn_fd << " Now client number:" << s_clientsList.size() << endl;

                m_user[conn_fd].address = client_addr;
                m_user[conn_fd].sockfd = conn_fd;

                // 创建基于升序链表的定时器
                // util_timer *timer = new util_timer;
                // timer->userData = &m_user[conn_fd];  //设置用户数据
                // timer->callBackFunc = TimerCallBack; //设置回调函数
                // time_t cur = time(nullptr);    //获取当前时间
                // timer->expire = cur + 3 * TIMESLOT;  //设置客户端活动时间
                // m_user[conn_fd].timer = timer;       //绑定定时器
                // m_timerList.AddTimer(timer);         //将定时器添加到升序链表中

                // 创建时间轮定时器
                // tw_timer *timer = m_timerWheel.AddTimer(30);
                // timer->user_data = &m_user[conn_fd];
                // timer->callBackFunc = TimerCallBack;
                // m_user[conn_fd].timer = timer;

                // 创建时间堆定时器
                heap_timer *timer = new heap_timer(TIMESLOT);
                timer->user_data = &m_user[conn_fd];
                timer->callBackFunc = TimerCallBack;
                m_timerHeap.AddTimer(timer);
                if (!s_isAlarm) {
                    s_isAlarm = true;
                    alarm(TIMESLOT);
                }
                m_user[conn_fd].timer = timer;

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

                    memset(m_user[sockfd].buf, '\0', BUFFER_SIZE);
                    int recvRet = recv(sockfd, m_user[sockfd].buf, BUFFER_SIZE - 1, 0);
                    // util_timer *timer = m_user[sockfd].timer;
                    // tw_timer *timer = m_user[sockfd].timer;
                    heap_timer *timer = m_user[sockfd].timer;

                    // char client_buf[BUFFER_SIZE];
                    // memset(&client_buf, '\0', BUFFER_SIZE);
                    // int recvRet = recv(sockfd, client_buf, BUFFER_SIZE - 1, 0);
                    
                    if (recvRet == 0) { //对端正常关闭socket
                        TimerCallBack(&m_user[sockfd]);
                        if (timer) {
                            // m_timerList.DelTimer(timer);
                            // m_timerWheel.DelTimer(timer);
                            m_timerHeap.DelTimer(timer);
                        }
                        s_clientsList.remove(sockfd);
                        cout << "Server: Client close socket!" << endl;
                        cout << "Server: Now client number:" << s_clientsList.size() << endl;
                        break;
                    } else if (recvRet < 0){
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) { //读完全部数据
                            BroadcastMsg(sockfd, m_recvStr);
                            break;
                        }
                        if ((errno != EAGAIN) && (errno != EINTR)) { //对端异常断开socket
                            TimerCallBack(&m_user[sockfd]);
                            if (timer) {
                                // m_timerList.DelTimer(timer);
                                // m_timerWheel.DelTimer(timer);
                                m_timerHeap.DelTimer(timer);
                            }
                            s_clientsList.remove(sockfd);
                            cout << "Server: Client abnormal close socket!" << endl;
                            cout << "Server: Now client number:" << s_clientsList.size() << endl;
                            break;
                        }
                    } else { //读到一次数据
                        // 更新升序链表定时器
                        // if (timer) {
                        //     time_t cur = time(nullptr);
                        //     timer->expire = cur + 3 * TIMESLOT;
                        //     m_timerList.AdjustTimer(timer);
                        // }

                        // 更新时间轮定时器
                        // if (timer) timer->rotation++;

                        // 更新时间堆定时器
                        if (timer) {
                            timer->expire += TIMESLOT;
                            // 更新定时器后，需要设置当前没有进行定时任务
                            // 从而在 TimerHandler() 中重新触发 SIGALRM 信号
                            s_isAlarm = false;
                        }

                        m_recvStr.append(m_user[sockfd].buf);
                        cout << "Server: Recv data: " << m_user[sockfd].buf << endl; 
                    }
                }
            } else {
                cout << "Server: socket something else happened!" << endl;
            }
        }

        if (m_timeout) {
            TimerHandler();
            m_timeout = false;
        }
    }

    cout << "CLOSE SERVER!" << endl;
    close(m_socketFd);
    close(s_epollFd);
    close(s_pipeFd[0]);
    close(s_pipeFd[1]);
}

