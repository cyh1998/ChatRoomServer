//
// Created by cyh on 2021/3/11.
//

#ifndef CHATROOMSERVER_THREADOBJECT_H
#define CHATROOMSERVER_THREADOBJECT_H

#include <functional>
#include <pthread.h>

class ThreadObject {
public:
    typedef std::function<void ()> ThreadFunc;

    explicit ThreadObject(ThreadFunc);
    ~ThreadObject();

    void start();
    void join();
    void cancel();
    bool started() const { return m_isStarted; }

    static void* run(void *obj);

private:
    pthread_t m_pthreadId;
    bool m_isStarted;
    bool m_isJoined;
    ThreadFunc m_func;
};

#endif //CHATROOMSERVER_THREADOBJECT_H
