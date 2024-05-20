/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
#include "Semaphore.h"
#include <iostream>
#include <chrono>


using namespace std;


void ThreadPool::dispatcher(){
    while(true){
        sem.wait();
        thunks_lock.lock();
        function<void(void)> thunk = thunks.front();
        thunks.pop();
        thunks_lock.unlock();
        for (size_t i = 0; i < wts.size(); i++) {
            if (wts[i].busy == false) {
                printf("voy a asignar una tarea al worker %d\n", i);
                wts[i].busy = true;
                wts[i].thunk = thunk;
                lock_guard<mutex> lock(wts[i].mtx);
                wts[i].cv.notify_all(); 
                break;
            }
        }
    }
}

void ThreadPool::worker(size_t ID) {
    while (true) {
        unique_lock<mutex> lock(wts[ID].mtx);
        wts[ID].cv.wait(lock, [this, ID] { return wts[ID].busy; });
        if (wts[ID].thunk) {
            wts[ID].thunk();
        }
        wts[ID].busy = false;
        printf("el valor de task_counter es %d\n", task_counter);
    }
}

ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads) {

    printf("creo el thread pool\n");
    dt = thread([this](){dispatcher();});
    for (size_t i = 0; i < numThreads; i++) {
        wts[i].ID = i;
        wts[i].ts = thread([this, i](){worker(i);}); 
    }

}
void ThreadPool::schedule(const function<void(void)>& thunk) {
    printf("voy a poner una tarea en el pool\n");
    thunks_lock.lock();
    thunks.push(thunk);
    thunks_lock.unlock();
    sem.signal(); //avisale al compa dispatcher
}

void ThreadPool::wait() {
    // while (task_counter > 0) {
        // printf("esperando\n");
        this_thread::sleep_for(chrono::milliseconds(5000));
    // }
}

ThreadPool::~ThreadPool() {

}