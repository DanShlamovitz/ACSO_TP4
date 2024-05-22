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
    //espera a que le avisen para trabajar. cuando trabaja se fija si queda algo por hacer y sino le avisa
    //a la cv del wait. Al final le avisa al semaforo del dispatcher q hay un worker más libre
    while (true) {
        wts[ID].worker_sem.wait();  
        if (finished) break;
        wts[ID].thunk();
        wts[ID].busy = false;
        {
            unique_lock<mutex> lock(thunks_lock);
            work_count--;
            if (work_count == 0) {
                cv.notify_all();
            }
        }
        wsem.signal();
    }
}

void ThreadPool::dispatcher() {
    //cuando le avise el schedule y ademas haya un trabajador para darle trabajo saca el trabajo busca al trabajador disponible y se lo da para trabajar
    while (true) {
        sem.wait();
        if (finished) break;
        wsem.wait();
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
ThreadPool::ThreadPool(size_t numThreads)  : wts(numThreads),sem(0), wsem(numThreads),  work_count(0){
    //creo los threads
    dt = thread([this](){ dispatcher(); });
    for (size_t i = 0; i < numThreads; i++) {
        wts[i].ID = i;
        wts[i].busy = false;
        wts[i].ts = thread([this, i](){ worker(i); });
    }
}
void ThreadPool::schedule(const function<void(void)>& thunk) {
    //saco pongo la función en la cola y le aviso al dispatcher que hay algo nuevo en la cola para que se desbloquee
    thunks_lock.lock(); 
    thunks.push(thunk);
    work_count++;
    sem.signal();
    thunks_lock.unlock();
}
void ThreadPool::wait() {
    //espera a que los workers terminen de trabajar utilizando una cv que solo la señalizan lso workers cuando termino de trabajar el ultimo
    // cout << "Wait \n";
    unique_lock<mutex> lock(thunks_lock);
    while (work_count > 0) {
        cv.wait(lock);
    }
}

ThreadPool::~ThreadPool() {
    //espera a que terminen de trabajartodos y une el trabajo.
    // cout << "Destructor \n";
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


