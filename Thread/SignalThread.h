//
// Created by cyh on 2021/3/15.
//

#ifndef CHATROOMSERVER_SIGNALTHREAD_H
#define CHATROOMSERVER_SIGNALTHREAD_H

#include <signal.h>
#include "ThreadObject.h"

class SignalThread : public ThreadObject {
public:
    explicit SignalThread();
    ~SignalThread() override;

    void setSignalSet(sigset_t *set);

private:
    sigset_t *m_set;
    void *run() override;
};

#endif //CHATROOMSERVER_SIGNALTHREAD_H
