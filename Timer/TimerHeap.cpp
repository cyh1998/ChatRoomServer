//
// Created by cyh on 2021/2/19.
//

#include "TimerHeap.h"

TimerHeap::TimerHeap() = default;

TimerHeap::~TimerHeap() {
    while (!m_timer_pqueue.empty()) {
        delete m_timer_pqueue.top();
        m_timer_pqueue.pop();
    }
}

void TimerHeap::AddTimer(heap_timer *timer) {
    if (!timer) return;
    m_timer_pqueue.emplace(timer);
}

void TimerHeap::DelTimer(heap_timer *timer) {
    if (!timer) return;
    // 将目标定时器的回调函数设为空，即延时销毁
    // 减少删除定时器的开销，但会扩大优先级队列的大小
    timer->callBackFunc = nullptr;
}

void TimerHeap::Tick() {
    time_t cur = time(nullptr);
    while (!m_timer_pqueue.empty()) {
        heap_timer *timer = m_timer_pqueue.top();
        //堆顶定时器没有到期，退出循环
        if (timer->expire > cur) break;
        //否则执行堆顶定时器中的任务
        if (timer->callBackFunc) timer->callBackFunc(timer->user_data);
        m_timer_pqueue.pop();
    }
}

heap_timer *TimerHeap::Top() {
    if (m_timer_pqueue.empty()) return nullptr;
    else return m_timer_pqueue.top();
}

