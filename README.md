# Thread Pool Implementation in C

## Basic explanation

A thread pool is a software design pattern for achieving concurrency of execution in a computer program. Often also called a replicated workers or worker-crew model, a thread pool maintains multiple threads waiting for tasks to be allocated for concurrent execution by the supervising program. By maintaining a pool of threads, the model increases performance and avoids latency in execution due to frequent creation and destruction of threads for short-lived tasks. The number of available threads is tuned to the computing resources available to the program, such as a parallel task queue after completion of execution. (Wikipedia)

## Implementation
My implementation uses a queue for tasks (the files for the queue implementation were supplied by my university and course staff).

This implementation supports the following actions:
 * Start a new thread pool with a desired number of threads.
 * Add a new task to the queue.
 * Stop the thread pool in a clean way which lets the threads keep going until the queue is empty (at this time, no other tasks could be added).
 * Stop the thread pool abruptly which stops the thread pool as soon as every thread is done with it's current task. In that option, all tasks that are still in the queue will be ignored.

To test and run the thread pool you can run the tests file (written by Ron Even).

