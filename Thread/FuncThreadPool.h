//
// Created by cyh on 2021/3/19.
//

#ifndef CHATROOMSERVER_FUNCTHREADPOOL_H
#define CHATROOMSERVER_FUNCTHREADPOOL_H

#include <iostream>
#include <functional>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>


class FuncThreadPool {
public:
    typedef std::function<void(void)> Task;

    explicit FuncThreadPool(int threadNumber = 5, int maxRequests = 10000);
    ~FuncThreadPool();

    bool append(Task task); //添加任务接口

private:
    static void *entryFunc(void *arg);
    void run();

private:
    int m_threadNumber; //线程数
    int m_maxRequests; //最大任务数
    std::queue<Task> m_workQueue; //任务队列
    std::mutex m_Mutex; //互斥量
    std::condition_variable m_CondVar; //条件变量
    bool m_stop; //线程池是否执行
};

#endif //CHATROOMSERVER_FUNCTHREADPOOL_H
