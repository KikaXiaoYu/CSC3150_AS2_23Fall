
#include <stdlib.h>
#include <pthread.h>
#include "async.h"
#include "utlist.h"
#include <stdio.h>

my_queue_t *pool;

void *threadpool_function(void *arg)
{
    my_queue_t *pool = (my_queue_t *)arg;
    my_item_t *pjob = NULL;

    while (1)
    {
        pthread_mutex_lock(&pool->mutex);
        while (pool->queue_cur_num == 0) // 线程启动时需要等待任务被添加到任务队列，否则后续操作有内存安全隐患
        {
            pthread_cond_wait(&pool->queue_not_empty, &pool->mutex);
        }
        pjob = pool->head; // 从任务队列中取任务
        pool->queue_cur_num--;
        if(pool->queue_cur_num == pool->queue_max_num)
		{
			pthread_cond_broadcast(&pool->queue_is_full);
		}
        if (pool->queue_cur_num == 0) // 取完判断队列是否为空
        {
            pool->head = pool->tail = NULL;
            pthread_cond_broadcast(&pool->queue_is_empty);
        }
        else
        {
            pool->head = pool->head->next;
        }
        pthread_mutex_unlock(&pool->mutex);

        pjob->func(pjob->arg);
        free(pjob);
        pjob = NULL;
    }
    printf("func break final\n");
}

void async_init(int num_threads)
{

    pool = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);

    printf("break0\n");
    pool->queue_cur_num = 0;
    pool->queue_max_num = 50;
    pool->head = NULL;
    pool->tail = NULL;

    printf("break1\n");

    pthread_mutex_init(&pool->mutex, NULL);
    // judge conditions
    pthread_cond_init(&pool->queue_is_full, NULL);
    pthread_cond_init(&pool->queue_not_full, NULL);
    pthread_cond_init(&pool->queue_is_empty, NULL);
    pthread_cond_init(&pool->queue_not_empty, NULL);
    printf("break2\n");

    pool->thread_num = num_threads;
    pool->pthread_ids = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);

    for (int i; i < pool->thread_num; i++)
    {
        pthread_create(&(pool->pthread_ids[i]), NULL, threadpool_function, (void *)pool);
    }
    printf("break3\n");

    return;
    /** TODO: create num_threads threads and initialize the thread pool **/
}

void async_run(void (*hanlder)(int), int args)
{
    printf("run break0\n");
    pthread_mutex_lock(&pool->mutex);
    while (pool->queue_cur_num == pool->queue_max_num)
    {
        pthread_cond_wait(&pool->queue_not_full, &pool->mutex);
    }

    my_item_t *pjob = (my_item_t *)malloc(sizeof(my_item_t));

    pjob->func = hanlder;
    pjob->arg = args;
    pjob->next == NULL;
    if (pool->head == NULL)
    {
        pool->head = pool->tail = pjob;
        pthread_cond_broadcast(&pool->queue_not_empty);
    }
    else
    {
        pool->tail->next = pjob;
        pool->tail = pjob;
    }

    pool->queue_cur_num++;

    pthread_mutex_unlock(&pool->mutex);

    /** TODO: rewrite it to support thread pool **/
}