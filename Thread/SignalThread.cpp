//
// Created by cyh on 2021/3/15.
//
#include <iostream>
#include "SignalThread.h"

SignalThread::SignalThread() = default;

SignalThread::~SignalThread() = default;

void SignalThread::setSignalSet(sigset_t *set) {
    m_set = set;
}

void *SignalThread::run() {
    int ret, sig;
    for (;;) {
        ret = sigwait(m_set, &sig);
        if (ret == 0) {
            std::cout << "SignalThread: Get signal: " << sig << std::endl;
        }
    }
}

