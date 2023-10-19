
#include <stdlib.h>
#include <pthread.h>
#include "async.h"
#include "utlist.h"
#include <stdio.h>

/* global define the thread pool */
my_queue_t *pool;

/* the function of each pthread */
void *foo(void *t)
{
    /* allocate memory for pointer */
    // printf("[foo%d] : Allocate memory for pointer\n", t);
    my_item_t *web_job = NULL;

    while (1)
    {
        pthread_mutex_lock(&pool->mutex);

        while (pool->cur_num == 0)
            // printf("[foo%d] : No job in the pool, waiting for it.\n", t);
            pthread_cond_wait(&pool->empty_false, &pool->mutex);

        // printf("[foo%d] : Fetching the job...\n", t);
        web_job = pool->head; // fetch the job
        // printf("[foo%d] : Fetch the job successfully!\n", t);

        /* after fetching, update the pool */
        // printf("[foo%d] : The current num of pool is %d.\n", t, pool->cur_num);
        if (web_job->next_pt == NULL) // if the queue will be empty
        {
            // printf("[foo%d] : The queue will be empty.\n", t);
            pool->head = NULL; // empty the head
            pool->tail = NULL; // empty the tail
            pthread_cond_broadcast(&pool->empty_true);
        }
        else // if the queue is not empty
        {
            // printf("[foo%d] : The queue is still not empty.\n", t);
            pool->head = web_job->next_pt;    // update the head item
            web_job->next_pt->prev_pt = NULL; // update the head's prev
            pthread_cond_broadcast(&pool->empty_false);
        }
        // printf("[foo%d] : Update the pool successfully!\n", t);

        pool->cur_num--; // update the queue number
        pthread_mutex_unlock(&pool->mutex);

        web_job->hanlder(web_job->args); // do the job

        /* free the memory of the job */
        web_job = NULL;
        // printf("[foo%d] : Job %d has been done!\n", t);
    }
}

void async_init(int num_threads)
{
    pool = (my_queue_t *)malloc(sizeof(my_queue_t));
    pool->cur_num = 0;
    pool->head = NULL;
    pool->tail = NULL;

    // printf("[Init] : Inialializing mutex and conditon variables...\n");
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->empty_true, NULL);
    pthread_cond_init(&pool->empty_false, NULL);

    // printf("[Init] : Initializing threads...\n");
    pool->pthreads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);

    for (int i = 0; i < num_threads; i++)
        // printf("[Init] : Creating pthread [%d]...\n", i);
        pthread_create(&(pool->pthreads[i]), NULL, foo, (void *)i);
    // printf("[Init] : Initialize the pool successfully!\n");

    return;
}

void async_run(void (*hanlder)(int), int args)
{
    pthread_mutex_lock(&pool->mutex);

    // printf("[Run] : Get a new job...\n");

    /* create a new job pointer, allocate memory */
    my_item_t *web_job = (my_item_t *)malloc(sizeof(my_item_t));

    web_job->hanlder = hanlder;
    web_job->args = args;

    // printf("[Run] : Update the pool...\n");
    // printf("[Run] : The current num of pool is %d.\n", pool->cur_num);
    /* update the pool */
    if (pool->head == NULL) // if the queue is empty
    {
        // printf("[Run] : No job in the pool.\n");
        pool->head = web_job;
        pool->tail = web_job;
        // printf("[Run] : Set head = tail = job.\n");
    }
    else // if the queue is not empty
    {
        // printf("[Run] : Some jobs in the pool.\n");
        web_job->prev_pt = pool->tail;
        pool->tail->next_pt = web_job;
        pool->tail = web_job;
        // printf("[Run] : Update the pool.\n");
    }

    pthread_cond_broadcast(&pool->empty_false);
    // printf("[Run] : Pool in the job successfully!\n");

    /* update the queue number */
    pool->cur_num++;
    // printf("[Run] : The current num of pool is %d.\n", pool->cur_num);

    pthread_mutex_unlock(&pool->mutex);

    return;
}