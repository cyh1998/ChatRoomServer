//
// Created by cyh on 2021/3/19.
//

#include "FuncThreadPool.h"

FuncThreadPool::FuncThreadPool(int threadNumber, int maxRequests) :
    m_threadNumber(threadNumber),
    m_maxRequests(maxRequests),
    m_stop(false)
{
    if (m_threadNumber <= 0 || m_threadNumber > m_maxRequests) throw std::exception();

    for (int i = 0; i < m_threadNumber; ++i) {
        std::cout << "FuncThreadPool: Create " << i + 1 << " thread" << std::endl;
        std::thread([this] { FuncThreadPool::entryFunc(this); }).detach(); //创建并分离线程
    }
}

FuncThreadPool::~FuncThreadPool() {
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_stop = true;
    }
    m_CondVar.notify_all(); //通知所有线程停止
}

bool FuncThreadPool::append(FuncThreadPool::Task task) {
    if (m_workQueue.size() > m_maxRequests) {
        std::cout << "FuncThreadPool: Work queue is full" << std::endl;
        return false;
    }
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_workQueue.emplace(task);
    }
    m_CondVar.notify_one(); //通知线程
    return true;
}

void *FuncThreadPool::entryFunc(void *arg) {
    FuncThreadPool *ptr = static_cast<FuncThreadPool *>(arg);
    ptr->run();
    return nullptr;
}

void FuncThreadPool::run() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    while (!m_stop) {
        m_CondVar.wait(lock);
        if (!m_workQueue.empty()) {
            Task task = m_workQueue.front();
            m_workQueue.pop();
            task();
        }
    }
}
