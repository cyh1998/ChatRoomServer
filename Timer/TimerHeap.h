//
// Created by cyh on 2021/2/19.
//

#ifndef CHATROOMSERVER_TIMERHEAP_H
#define CHATROOMSERVER_TIMERHEAP_H

#include <netinet/in.h>
#include <functional>
#include <vector>
#include <queue>

#define BUFFER_SIZE 0xFFFF  //缓存区数据大小
#define TIMESLOT    30      //定时时间

class heap_timer; //前向声明

// 客户端数据
struct client_data {
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    heap_timer *timer;
};

// 定时器类
class heap_timer {
public:
    heap_timer(int delay) {
        expire = time(nullptr) + delay;
    }

public:
    time_t expire; //定时器生效的绝对时间
    std::function<void(client_data *)> callBackFunc; //回调函数
    client_data *user_data; //用户数据
};

struct cmp { //比较函数，实现小根堆
    bool operator () (const heap_timer* a, const heap_timer* b) {
        return a->expire > b->expire;
    }
};

class TimerHeap {
public:
    explicit TimerHeap();
    ~TimerHeap();

public:
    void AddTimer(heap_timer *timer); //添加定时器
    void DelTimer(heap_timer *timer); //删除定时器
    void Tick(); //心搏函数
    heap_timer* Top();

private:
    std::priority_queue<heap_timer *, std::vector<heap_timer *>, cmp> m_timer_pqueue; //时间堆
};

#endif //CHATROOMSERVER_TIMERHEAP_H

