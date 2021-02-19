//
// Created by cyh on 2021/2/18.
//

#ifndef CHATROOMSERVER_TIMERWHEEL_H
#define CHATROOMSERVER_TIMERWHEEL_H

#include <netinet/in.h> //sockaddr_in
#include <list>
#include <functional>
#include <unordered_map>

#define BUFFER_SIZE 0xFFFF  //缓存区数据大小
#define TIMESLOT    1       //定时时间

class tw_timer; //前向声明

//客户端数据
struct client_data {
    sockaddr_in address; //socket地址
    int sockfd; //socket文件描述符
    char buf[BUFFER_SIZE]; //数据缓存区
    tw_timer *timer; //定时器
};

//定时器类
class tw_timer {
public:
    tw_timer(int rot, int ts) : rotation(rot), time_slot(ts){}

public:
    int rotation; //记录定时器在时间轮转多少圈后生效
    int time_slot; //记录定时器属于时间轮上的哪个槽
    std::function<void(client_data *)> callBackFunc; //回调函数
    client_data *user_data; //用户数据
};

class TimerWheel {
public:
    explicit TimerWheel();
    ~TimerWheel();

public:
    tw_timer* AddTimer(int timeout); //根据超时时间 timeout 添加定时器
    void DelTimer(tw_timer *timer); //删除定时器
    void Tick(); //心搏函数

private:
    static const int N = 60; //时间轮上槽的数量
    static const int SI = TIMESLOT; //每 1s 时间轮转动一次，即槽间隔为 1s
    int m_cur_slot; //时间轮的当前槽
    std::unordered_map<int, std::list<tw_timer *>> m_slots; //时间轮的槽，其中每个元素指向一个定时器链表
};

#endif //CHATROOMSERVER_TIMERWHEEL_H

