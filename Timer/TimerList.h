//
// Created by cyh on 2021/2/9.
//

#ifndef CHATROOMSERVER_TIMERLIST_H
#define CHATROOMSERVER_TIMERLIST_H

#include <netinet/in.h> //sockaddr_in
#include <list>
#include <functional>

#define BUFFER_SIZE 0xFFFF  //缓存区数据大小
#define TIMESLOT    20      //定时时间

class util_timer; //前向声明

//客户端数据
struct client_data {
    sockaddr_in address; //socket地址
    int sockfd; //socket文件描述符
    char buf[BUFFER_SIZE]; //数据缓存区
    util_timer *timer; //定时器
};

//定时器类
class util_timer {
public:
    time_t expire; //任务超时时间(绝对时间)
    std::function<void(client_data *)> callBackFunc; //回调函数
    client_data *userData; //用户数据
};

class TimerList {
public:
    explicit TimerList();
    ~TimerList();

public:
    void AddTimer(util_timer *timer); //添加定时器
    void DelTimer(util_timer *timer); //删除定时器
    void AdjustTimer(util_timer *timer); //调整定时器
    void Tick(); //处理链表上到期的任务

private:
    std::list<util_timer *> m_timer_list; //定时器链表
};


#endif //CHATROOMSERVER_TIMERLIST_H
