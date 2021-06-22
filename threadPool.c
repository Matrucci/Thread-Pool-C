// Matan Saloniko 318570769

#include "threadPool.h"
#include <stdlib.h>
#include <stdio.h>

#define TRUE 1

void runThread(void* tp) {
    ThreadPool* t = (ThreadPool*) tp;
    ThreadTask* threadTask;

    while (TRUE) {
        pthread_mutex_lock(&t->mutex);
        while (t->numOfTasks == 0 && t->isDestroyed == 0) {
            pthread_cond_wait(&t->wait, &t->mutex);
        }
        if (t->isDestroyed && t->isWaitingForTasks == 0) {
            break;
        }
        if (t->isDestroyed && t->isWaitingForTasks && t->numOfTasks == 0) {
            break;
        }
        threadTask = (ThreadTask*) osDequeue(t->tasks);
        t->activeThreads++;
        pthread_mutex_unlock(&t->mutex);
        //Doing the task
        if (threadTask != NULL) {
            threadTask->function(threadTask->args);
            free(threadTask);
            t->numOfTasks--;
        }

        pthread_mutex_lock(&t->mutex);
        t->activeThreads--;
        if (t->isDestroyed == 0 && t->activeThreads == 0 && t->numOfTasks == 0) {
            pthread_cond_signal(&t->waitCon);
        }
        pthread_mutex_unlock(&t->mutex);
    }
    t->theadCount--;
    pthread_cond_signal(&(t->waitCon));
    pthread_mutex_unlock(&t->mutex);
    pthread_exit(NULL);
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
    pthread_cond_init(&t->wait, NULL);
    pthread_cond_init(&t->waitCon, NULL);

    //Creating the threads.
    int i, retCode;
    for (i = 0; i < t->theadCount; i++) {
        retCode = pthread_create(&(t->threads[i]), NULL, (void*)runThread, t);
        if (retCode != 0) {
            perror("Error in pthread_create");
            osDestroyQueue(t->tasks);
            pthread_mutex_destroy(&t->mutex);
            pthread_cond_destroy(&t->wait);
            pthread_cond_destroy(&t->waitCon);
            free(t->threads);
            free(t);
            return NULL;
        }
        pthread_detach(t->threads[i]);
    }
    return t;
}

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {
    if (threadPool == NULL) {
        return;
    }

    if (threadPool->isDestroyed) {
        return;
    }

    ThreadTask * threadTask;
    pthread_mutex_lock(&threadPool->mutex);
    threadPool->isWaitingForTasks = shouldWaitForTasks;
    if (shouldWaitForTasks == 0) {
        threadTask = osDequeue(threadPool->tasks);
        while (threadTask != NULL) {
            free(threadTask);
            threadTask = osDequeue(threadPool->tasks);
        }
    } else {
        /*
        threadTask = osDequeue(threadPool->tasks);
        while (threadTask != NULL)
        {
            threadPool->numOfTasks--;
            threadPool->theadCount--;
            pthread_mutex_lock(&(threadPool->mutex));
            pthread_cond_wait(&(threadPool->waitCon), &(threadPool->mutex));

            threadPool->activeThreads++;
            pthread_mutex_unlock(&(threadPool->mutex));
            threadTask = osDequeue(threadPool->tasks);
        }
*/
    }
    threadPool->isDestroyed = 1;
    pthread_cond_broadcast(&(threadPool->wait));


    while (TRUE) {
        if (threadPool->activeThreads != 0) {
            pthread_cond_wait(&threadPool->waitCon, &threadPool->mutex);
        } else {
            break;
        }
    }

    if (threadPool->theadCount) {
        free(threadPool->threads);
        osDestroyQueue(threadPool->tasks);
        pthread_mutex_destroy(&threadPool->mutex);
        pthread_cond_destroy(&threadPool->waitCon);
        pthread_cond_destroy(&threadPool->wait);
    }
    free(threadPool);
}

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param) {
    if (threadPool == NULL) {
        return -1;
    }
    if (computeFunc == NULL) {
        return -1;
    }

    pthread_mutex_lock(&threadPool->mutex);
    if (threadPool->isDestroyed) {
        return -1;
    }
    ThreadTask* threadTask = (ThreadTask*)malloc(sizeof(ThreadTask));
    threadTask->function = computeFunc;
    threadTask->args = param;
    osEnqueue(threadPool->tasks, threadTask);
    threadPool->numOfTasks++;
    pthread_cond_signal(&threadPool->wait);
    pthread_mutex_unlock(&threadPool->mutex);
    return 0;
}