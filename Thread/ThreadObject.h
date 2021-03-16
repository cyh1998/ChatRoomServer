//
// Created by cyh on 2021/3/11.
//

#ifndef CHATROOMSERVER_THREADOBJECT_H
#define CHATROOMSERVER_THREADOBJECT_H

#include <pthread.h>

class ThreadObject {
public:
    explicit ThreadObject();
    virtual ~ThreadObject();

private:
    pthread_t m_pthreadId;
    bool m_isStarted;
    bool m_isJoined;

    static void* entryFunc(void *obj);
    virtual void* run() = 0;

public:
    void start();
    void join();
    void cancel();
    bool started() const { return m_isStarted; }
};

#endif //CHATROOMSERVER_THREADOBJECT_H
