//
// Created by cyh on 2021/2/9.
//
#include "TimerList.h"

TimerList::TimerList() = default;

TimerList::~TimerList() {
    m_timer_list.clear();
}

void TimerList::AddTimer(util_timer *timer) {
    if (!timer) return;
    else {
        auto item = m_timer_list.begin();
        while (item != m_timer_list.end()) {
            if (timer->expire < (*item)->expire) {
                m_timer_list.insert(item, timer);
                return;
            }
            item++;
        }
        m_timer_list.emplace_back(timer);
    }
}

void TimerList::DelTimer(util_timer *timer) {
    if (!timer) return;
    else {
        auto item = m_timer_list.begin();
        while (item != m_timer_list.end()) {
            if (timer == *item) {
                m_timer_list.erase(item);
                m_timer_list.remove(timer);
                return;
            }
            item++;
        }
    }
}

void TimerList::AdjustTimer(util_timer *timer) {
    DelTimer(timer);
    AddTimer(timer);
}

void TimerList::Tick() {
    if (m_timer_list.empty()) return;
    time_t cur = time(nullptr);

    while (!m_timer_list.empty()) {
        util_timer *temp = m_timer_list.front();
        if (cur < temp->expire) break;
        temp->callBackFunc(temp->userData);
        m_timer_list.pop_front();
    }
}
