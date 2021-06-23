// Matan Saloniko 318570769

#include "threadPool.h"
#include <stdlib.h>
#include <stdio.h>

#define TRUE 1

#define MUTEX_ERROR "Error in mutex"
#define WAIT_ERROR "Error in cond wait"
#define SIGNAL_ERROR "Error in signal"
#define MALLOC_ERROR "Error in malloc"
#define QUEUE_ERROR "Can not create task queue"
#define THREAD_ERROR "Error while creating the threads"
#define MUTEX_CREATE_ERROR "Error while creating mutex"
#define COND_CREATE_ERROR "Error while creating cond"
#define DETACH_ERROR "Error while detaching threads"
#define TASK_ERROR "Error while creating a task"

/****************************************************
 * Freeing the threadpool and all of it's variables.
 * @param threadPool
 ***************************************************/
void freeThreadPool(ThreadPool* threadPool) {
    if (threadPool != NULL) {
        pthread_mutex_lock(&(threadPool->mutex));
        free(threadPool->threads);
        while (!osIsQueueEmpty(threadPool->tasks)) {
            free(osDequeue(threadPool->tasks));
        }
        osDestroyQueue(threadPool->tasks);
        pthread_mutex_destroy(&(threadPool->mutex));
        pthread_cond_destroy(&(threadPool->waitCon));
        pthread_cond_destroy(&(threadPool->wait));
        free(threadPool);
    }
}

/**************************************
 * The function that each thread runs.
 * @param tp - The threadpool
 *************************************/
void runThread(void* tp) {
    ThreadPool* t = (ThreadPool*) tp;
    ThreadTask* threadTask;

    //Running until the threadpool is destroyed.
    while (TRUE) {
        if (pthread_mutex_lock(&(t->mutex)) != 0) {
            freeThreadPool(tp);
            perror(MUTEX_ERROR);
            exit(-1);
        }

        //Waiting for a task to be added.
        while (t->numOfTasks == 0 && t->isDestroyed == 0) {
            if (pthread_cond_wait(&(t->wait), &(t->mutex)) != 0) {
                freeThreadPool(tp);
                perror(WAIT_ERROR);
                exit(-1);
            }
        }
        //If the threadpool is destroyed.
        if (t->isDestroyed && t->isWaitingForTasks == 0) {
            break;
        }
        if (t->isDestroyed && t->isWaitingForTasks && t->numOfTasks == 0) {
            break;
        }
        //Getting a task from the queue.
        threadTask = (ThreadTask*) osDequeue(t->tasks);
        t->activeThreads++;
        t->numOfTasks--;
        if (pthread_mutex_unlock(&(t->mutex)) != 0) {
            freeThreadPool(tp);
            perror(MUTEX_ERROR);
            exit(-1);
        }
        //Doing the task
        if (threadTask != NULL) {
            threadTask->function(threadTask->args);
            free(threadTask);
        }

        //Updating the active threads.
        if (pthread_mutex_lock(&(t->mutex)) != 0) {
            freeThreadPool(tp);
            perror(MUTEX_ERROR);
            exit(-1);
        }
        t->activeThreads--;
        if (pthread_mutex_unlock(&(t->mutex)) != 0) {
            freeThreadPool(tp);
            perror(MUTEX_ERROR);
            exit(-1);
        }
    }
    t->theadCount--;
    if (pthread_cond_signal(&(t->waitCon)) != 0) {
        freeThreadPool(tp);
        perror(SIGNAL_ERROR);
        exit(-1);
    }
    if (pthread_mutex_unlock(&(t->mutex)) != 0) {
        freeThreadPool(tp);
        perror(MUTEX_ERROR);
        exit(-1);
    }
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
    if (t->threads == NULL) {
        free(t);
        perror(MALLOC_ERROR);
        exit(-1);
    }
    t->activeThreads = 0;
    t->isDestroyed = 0;
    t->isWaitingForTasks = 0;
    t->tasks = osCreateQueue();
    if (t->tasks == NULL) {
        free(t->threads);
        free(t);
        perror(QUEUE_ERROR);
        exit(-1);
    }
    t->numOfTasks = 0;

    if (pthread_mutex_init(&(t->mutex), NULL) != 0) {
        osDestroyQueue(t->tasks);
        free(t->threads);
        free(t);
        perror(MUTEX_CREATE_ERROR);
        exit(-1);
    }
    if (pthread_cond_init(&(t->wait), NULL) != 0) {
        pthread_mutex_destroy(&(t->mutex));
        osDestroyQueue(t->tasks);
        free(t->threads);
        free(t);
        perror(COND_CREATE_ERROR);
        exit(-1);
    }
    if (pthread_cond_init(&(t->waitCon), NULL) != 0) {
        pthread_cond_destroy(&(t->wait));
        pthread_mutex_destroy(&(t->mutex));
        osDestroyQueue(t->tasks);
        free(t->threads);
        free(t);
        perror(COND_CREATE_ERROR);
        exit(-1);
    }

    //Creating the threads.
    int i, retCode;
    for (i = 0; i < t->theadCount; i++) {
        retCode = pthread_create(&(t->threads[i]), NULL, (void*)runThread, t);
        if (retCode != 0) {
            freeThreadPool(t);
            perror(THREAD_ERROR);
            exit(-1);
        }
        if (pthread_detach(t->threads[i]) != 0) {
            freeThreadPool(t);
            perror(DETACH_ERROR);
            exit(-1);
        }
    }
    return t;
}

/************************************************************************************
 * Destroying the threadpool.
 * @param threadPool
 * @param shouldWaitForTasks - 0 if we shouldn't wait
 *                    any other number if we should wait for everything to finish.
 ***********************************************************************************/
void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {
    if (threadPool == NULL) {
        return;
    }
    //If it's already destroyed.
    if (threadPool->isDestroyed) {
        return;
    }

    if (pthread_mutex_lock(&(threadPool->mutex)) != 0) {
        freeThreadPool(threadPool);
        perror(MUTEX_ERROR);
        exit(-1);
    }
    threadPool->isWaitingForTasks = shouldWaitForTasks;
    threadPool->isDestroyed = 1;
    //If we shouldn't wait, clear the queue.
    if (shouldWaitForTasks == 0) {
        threadPool->numOfTasks = 0;
        while (!osIsQueueEmpty(threadPool->tasks)) {
            free(osDequeue(threadPool->tasks));
        }
    }
    if (pthread_cond_broadcast(&(threadPool->wait)) != 0) {
        freeThreadPool(threadPool);
        perror(SIGNAL_ERROR);
        exit(-1);
    }

    //Waiting for all threads to finish their work.
    while (TRUE) {
        if (threadPool->theadCount != 0) {
            if (pthread_cond_wait(&(threadPool->waitCon), &(threadPool->mutex)) != 0) {
                freeThreadPool(threadPool);
                perror(WAIT_ERROR);
                exit(-1);
            }
        } else {
            break;
        }
    }
    if (pthread_mutex_unlock(&threadPool->mutex) != 0) {
        free(threadPool);
        perror(MUTEX_ERROR);
        exit(-1);
    }
    //Freeing the threadpool.
    freeThreadPool(threadPool);
}


/******************************************************************
 * Inserting a task to the threadpool.
 * @param threadPool
 * @param computeFunc - A pointer to a function.
 * @param param - The parameter to the function.
 * @return - 0 if evetything went fine. -1 if there was an error.
 *****************************************************************/
int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param) {
    if (threadPool == NULL) {
        return -1;
    }
    if (computeFunc == NULL) {
        return -1;
    }

    if (pthread_mutex_lock(&threadPool->mutex) != 0) {
        free(threadPool);
        perror(MUTEX_ERROR);
        exit(-1);
    }
    //If the threadpool has already been destroyed, we can't insert another task.
    if (threadPool->isDestroyed) {
        return -1;
    }
    //Creating a new task object.
    ThreadTask* threadTask = (ThreadTask*)malloc(sizeof(ThreadTask));
    if (threadTask == NULL) {
        freeThreadPool(threadPool);
        perror(TASK_ERROR);
        exit(-1);
    }
    threadTask->function = computeFunc;
    threadTask->args = param;
    osEnqueue(threadPool->tasks, threadTask);
    threadPool->numOfTasks++;
    //Updating the threads.
    if (pthread_cond_signal(&threadPool->wait) != 0) {
        freeThreadPool(threadPool);
        perror(WAIT_ERROR);
        exit(-1);
    }
    if (pthread_mutex_unlock(&threadPool->mutex) != 0) {
        freeThreadPool(threadPool);
        perror(MUTEX_ERROR);
        exit(-1);
    }
    return 0;
}