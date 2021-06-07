#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "osqueue.h"
#include "threadPool.h"


void hello (void* a)
{
    sleep(2);
    printf("hello\n");
}

void test_thread_pool_sanity(int threads_count, int task_count, int should_wait)
{
   int i;

   ThreadPool* tp = tpCreate(threads_count);
    if (tp == NULL)
    {
        printf("tpCreate failed\n");
        return;
    }

   for(i=0; i<task_count; ++i)
   {
       if (tpInsertTask(tp,hello,NULL) != 0)
       {
           printf("tpInsertTask failed\n");
           break;
       }
   }

   sleep(1);

   tpDestroy(tp,should_wait);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Not enough arguments\n");
    }

    int threads_count = atoi(argv[1]);
    int tasks_count = atoi(argv[2]);
    int should_wait = atoi(argv[3]);

    test_thread_pool_sanity(threads_count, tasks_count, should_wait);
    return 0;
}
