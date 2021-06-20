// Matan Saloniko 318570769
#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include "osqueue.h"
#include <pthread.h>

typedef struct thread_task {
    void* function;
    void* args
} ThreadTask;

typedef struct thread_pool
{
    //Thread pool variables.
    int theadCount;
    int activeThreads;
    int numOfTasks;

    //Thread pool flags.
    int isDestroyed;
    int isWaitingForTasks;

    //Thread pool functionality variables.
    pthread_t* threads;
    OSQueue* tasks;
    pthread_mutex_t mutex;
    pthread_cond_t wait;
    pthread_cond_t waitCon;

} ThreadPool;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
