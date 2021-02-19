//
// Created by cyh on 2021/2/18.
//

#include "TimerWheel.h"

TimerWheel::TimerWheel() : m_cur_slot(0) {
    for (int i = 0; i < N; ++i) {
        m_slots[i] = std::list<tw_timer *>();
    }
}

TimerWheel::~TimerWheel() {
    for (int i = 0; i < N; ++i) {
        auto iter = m_slots[i].begin();
        while (iter != m_slots[i].end()) {
            delete *iter;
            iter++;
        }
    }
    m_slots.clear();
}

tw_timer *TimerWheel::AddTimer(int timeout) {
    if (timeout < 0) return nullptr;
    int ticks = timeout < SI ? 1 : timeout / SI;
    int rotation = ticks / N; //计算待插入的定时器在时间轮转动多少圈后触发
    int ts = (m_cur_slot + (ticks % N)) % N; //计算待插入的定时器应该插入到时间轮的哪个槽
    tw_timer* timer = new tw_timer(rotation, ts);
    m_slots[ts].push_front(timer);
    return timer;
}

void TimerWheel::DelTimer(tw_timer *timer) {
    if (!timer) return;
    int ts = timer->time_slot;
    m_slots[ts].remove(timer);
    delete timer;
}

void TimerWheel::Tick() {
    auto iter = m_slots[m_cur_slot].begin();
    while (iter != m_slots[m_cur_slot].end()) {
        if ((*iter)->rotation > 0) { //定时器的 ratation 值大于0，则在这一轮中不起作用
            (*iter)->rotation--;
            iter++;
        } else { //定时器已到期，执行定时任务，最后删除该定时器
            (*iter)->callBackFunc((*iter)->user_data);
            delete *iter;
            iter = m_slots[m_cur_slot].erase(iter);
        }
    }
    m_cur_slot = ++m_cur_slot % N;
}

