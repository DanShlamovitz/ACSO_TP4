/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */
#include "thread-pool.h"
#include "Semaphore.h"
#include <iostream>
#include <chrono>
#include <functional>
#include <queue>
#include <mutex>
#include <vector>
#include <thread>
using namespace std;

void ThreadPool::worker(size_t ID) {
    while (true) {
        wts[ID].worker_sem.wait();  
        if (finished) break;
        wts[ID].thunk();
        wts[ID].busy = false;
        work_count --;
        wsem.signal();
    }
}

void ThreadPool::dispatcher() {
    while (true) {
        sem.wait();
        wsem.wait();
        if (finished) break;
        function<void(void)> thunk;
        { // cuando cierro desbloqueo
            unique_lock<mutex> lock(thunks_lock);
            if (!thunks.empty()) {
                thunk = thunks.front();
                thunks.pop();
            }
        }
        for (size_t i = 0; i < wts.size(); i++) {
            if (!wts[i].busy) {
                    wts[i].busy = true;
                    wts[i].thunk = thunk;
                    wts[i].worker_sem.signal(); 
                    break;
            }
        }
        
    }
}
ThreadPool::ThreadPool(size_t numThreads)  : wts(numThreads), wsem(numThreads), sem(0),  work_count(0){
    dt = thread([this](){ dispatcher(); });
    for (size_t i = 0; i < numThreads; i++) {
        wts[i].ID = i;
        wts[i].busy = false;
        wts[i].ts = thread([this, i](){ worker(i); });
    }
}
void ThreadPool::schedule(const function<void(void)>& thunk) {
    thunks_lock.lock();
    thunks.push(thunk);
    work_count++;
    sem.signal();
    thunks_lock.unlock();
}
void ThreadPool::wait() {
    while (true){
        bool all_finished = true;
        {
            unique_lock<mutex> lock(thunks_lock);
            all_finished = (work_count == 0);
        }
        if (all_finished) return; 
    }
    return;
}
ThreadPool::~ThreadPool() {
    wait();
    finished = true;
    sem.signal();
    dt.join();
    for (size_t i = 0; i < wts.size(); i++){
        wts[i].worker_sem.signal();
    }
    for (size_t i = 0; i < wts.size(); i++) {
        wts[i].ts.join();
    }
}


