#ifndef __ASYNC__
#define __ASYNC__

#include <pthread.h>

typedef struct my_item
{
    void (*hanlder)(int);
    int args;
    struct my_item *next_pt;
    struct my_item *prev_pt;
} my_item_t;

typedef struct my_queue
{
    int thread_num;      // the thread number
    pthread_t *pthreads; // pthreads

    my_item_t *head; // head of queue
    my_item_t *tail; // tail of queue

    int cur_num; // current number of queue

    pthread_mutex_t mutex;    // mutex
    pthread_cond_t empty_true;  // queue is empty
    pthread_cond_t empty_false; // queue is not empty
} my_queue_t;

void async_init(int);
void async_run(void (*fx)(int), int args);

#endif
