//
// Created by cyh on 2021/3/19.
//

#ifndef CHATROOMSERVER_THREADPOOL_H
#define CHATROOMSERVER_THREADPOOL_H

#include <iostream>
#include <vector>
#include <queue>
#include <pthread.h>
#include <mutex>
#include <condition_variable>

template<typename T>
class ThreadPool {
public:
    explicit ThreadPool(int threadNumber = 5, int maxRequests = 10000);
    ~ThreadPool();

    bool append(T *request); //添加任务接口

private:
    static void *entryFunc(void *arg);
    void run();

private:
    int m_threadNumber; //线程数
    int m_maxRequests; //最大任务数
    std::queue<T *> m_workQueue; //任务队列
    std::mutex m_Mutex; //互斥量
    std::condition_variable m_CondVar; //条件变量
    bool m_stop; //线程池是否执行
};

template<typename T>
ThreadPool<T>::ThreadPool(int threadNumber, int maxRequests) :
    m_threadNumber(threadNumber),
    m_maxRequests(maxRequests),
    m_stop(false)
{
    if (m_threadNumber <= 0 || m_threadNumber > m_maxRequests) throw std::exception();

    for (int i = 0; i < m_threadNumber; ++i) {
        pthread_t pid;
        if (pthread_create(&pid, nullptr, entryFunc, static_cast<void*>(this)) == 0) { //创建线程
            std::cout << "ThreadPool: Create " << i + 1 << " thread" << std::endl;
            pthread_detach(pid); //分离线程
        }
    }
}

template<typename T>
ThreadPool<T>::~ThreadPool(){
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_stop = true;
    }
    m_CondVar.notify_all(); //通知所有线程停止
};

template<typename T>
bool ThreadPool<T>::append(T *request) {
    if (m_workQueue.size() > m_maxRequests) {
        std::cout << "ThreadPool: Work queue is full" << std::endl;
        return false;
    }
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_workQueue.emplace(request);
    }
    m_CondVar.notify_one(); //通知线程
    return true;
}

template<typename T>
void *ThreadPool<T>::entryFunc(void *arg) {
    ThreadPool *ptr = static_cast<ThreadPool *>(arg);
    ptr->run();
    return nullptr;
}

template<typename T>
void ThreadPool<T>::run() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    while (!m_stop) {
        m_CondVar.wait(lock);
        if (!m_workQueue.empty()) {
            T *request = m_workQueue.front();
            m_workQueue.pop();
            if (request) request->process();
        }
    }
}

#endif //CHATROOMSERVER_THREADPOOL_H
