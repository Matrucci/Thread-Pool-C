// Matan Saloniko 318570769

#include "threadPool.h"
#include <stdlib.h>
#include <stdio.h>


void runThread(void* tp) {
    ThreadPool* t = (ThreadPool*) tp;
    ThreadTask* threadTask;
    int flag = 1;

    while (flag) {
        pthread_mutex_lock(&t->mutex);
        while (t->numOfTasks == 0 && t->isDestroyed == 0) {
            pthread_cond_wait();
        }
        if (t->isDestroyed == 1 && t->isWaitingForTasks == 0) {
            flag = 0;
        }
    }
    pthread_mutex_lock(&t->mutex);
    t->theadCount--;
    pthread_mutex_unlock(&t->mutex);
}

/**************************************************************
 * Creates a thread pool with a given number of threads.
 * @param numOfThreads - The number of threads we want.
 * @return - A pointer to a struct representing a threadpool.
 *************************************************************/
ThreadPool* tpCreate(int numOfThreads) {
    if (numOfThreads < 1) {
        return NULL;
    }
    ThreadPool* t = (ThreadPool*)malloc(sizeof(ThreadPool));
    if (t == NULL) {
        perror("Error in malloc");
        return NULL;
    }

    //Setting arguments.
    t->theadCount = numOfThreads;
    t->threads = (pthread_t*)malloc(sizeof(pthread_t) * numOfThreads);
    t->activeThreads = 0;
    t->isDestroyed = 0;
    t->isWaitingForTasks = 0;
    t->tasks = osCreateQueue();
    t->numOfTasks = 0;

    pthread_mutex_init(&t->mutex, NULL);

    //Creating the threads.
    int i, retCode;
    for (i = 0; i < t->theadCount; i++) {
        retCode = pthread_create(&(t->threads[i]), NULL, (void*)runThread, t);
        if (retCode != 0) {
            perror("Error in pthread_create");
            osDestroyQueue(t->tasks);
            pthread_mutex_destroy(&t->mutex);
            free(t->threads);
            free(t);
            return NULL;
        }
        pthread_detach(t->threads[i]);
    }
    return t;
}

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {

}

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param) {

}