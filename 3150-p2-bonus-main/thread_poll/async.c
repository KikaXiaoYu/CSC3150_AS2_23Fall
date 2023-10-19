
#include <stdlib.h>
#include <pthread.h>
#include "async.h"
#include "utlist.h"
#include <stdio.h>

/* global define the thread pool */
my_queue_t pool;

/* the function of each pthread */
void *foo(void *t)
{
    /* allocate memory for pointer */
    my_item_t *web_job = (my_item_t *)malloc(sizeof(my_item_t));

    while (1)
    {
        pthread_mutex_lock(&pool.mutex);

        while (pool.cur_num == 0)
        {
            pthread_cond_wait(&pool.empty_false, &pool.mutex);
        }

        web_job = pool.head; // fetch the job

        /* after fetching, update the pool */
        if (web_job->next_pt == NULL) // if the queue will be empty
        {
            pool.head = NULL; // empty the head
            pool.tail = NULL; // empty the tail
            pthread_cond_broadcast(&pool.empty_true);
        }
        else // if the queue is not empty
        {
            pool.head = web_job->next_pt;     // update the head item
            web_job->next_pt->prev_pt = NULL; // update the head's prev
            pthread_cond_broadcast(&pool.empty_false);
        }

        pool.cur_num--; // update the queue number
        pthread_mutex_unlock(&pool.mutex);

        web_job->hanlder(web_job->args); // do the job

        /* free the memory of the job */
        free(web_job);
        web_job = NULL;
        printf("One job has been done!\n");
    }
}

void async_init(int num_threads)
{
    pool.cur_num = 0;
    pool.head = NULL;
    pool.tail = NULL;

    printf("Inialializing mutex and conditon variables...\n");
    pthread_mutex_init(&pool.mutex, NULL);
    pthread_cond_init(&pool.empty_true, NULL);
    pthread_cond_init(&pool.empty_false, NULL);

    printf("Initializing threads...\n");
    pool.thread_num = num_threads;
    pool.pthreads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);

    for (int i = 0; i < pool.thread_num; i++)
    {
        pthread_create(&(pool.pthreads[i]), NULL, foo, (void *)i);
    }

    return;
}

void async_run(void (*hanlder)(int), int args)
{
    pthread_mutex_lock(&pool.mutex);

    /* create a new job pointer, allocate memory */
    my_item_t *web_job = (my_item_t *)malloc(sizeof(my_item_t));
    web_job->hanlder = hanlder;
    web_job->args = args;

    /* update the pool */
    if (pool.head == NULL) // if the queue is empty
    {
        pool.head = web_job;
        pool.tail = web_job;
    }
    else // if the queue is not empty
    {
        web_job->prev_pt = pool.tail;
        pool.tail->next_pt = web_job;
        pool.tail = web_job;
    }
    pthread_cond_broadcast(&pool.empty_false);

    /* update the queue number */
    pool.cur_num++;

    pthread_mutex_unlock(&pool.mutex);

    return;
}