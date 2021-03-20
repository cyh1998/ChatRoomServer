//
// Created by cyh on 2021/3/11.
//

#include <iostream>
#include "ThreadObject.h"

ThreadObject::ThreadObject(ThreadFunc func) :
    m_pthreadId(0),
    m_isStarted(false),
    m_isJoined(false),
    m_func(std::move(func))
{

}

ThreadObject::~ThreadObject() {
    if (m_isStarted && !m_isJoined) {
        pthread_detach(m_pthreadId);
    }
}

void ThreadObject::start() {
    m_isStarted = true;
    if (pthread_create(&m_pthreadId, nullptr, run, static_cast<void*>(this))) {
        m_isStarted = false;
        std::cout << "ThreadObject: Create a new thread failed!" << std::endl;
    }
}

void ThreadObject::join() {
    if (m_isStarted && !m_isJoined) {
        m_isJoined = true;
        pthread_join(m_pthreadId, nullptr);
    }
}

void ThreadObject::cancel() {
    if (!pthread_cancel(m_pthreadId)) {
        m_isStarted = false;
    }
}

void* ThreadObject::run(void *obj) {
    ThreadObject* ptr = static_cast<ThreadObject*>(obj);
    ptr->m_func();
    delete ptr;
    return nullptr;
}
