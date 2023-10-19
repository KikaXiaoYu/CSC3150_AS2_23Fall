#ifndef __ASYNC__
#define __ASYNC__

#include <pthread.h>

typedef struct my_item
{
  void *(*func)(void *arg);
  void *arg;
  struct my_item *next;
  struct my_item *prev;
} my_item_t;

typedef struct my_queue
{
  int thread_num;         // the thread number
  pthread_t *pthread_ids; // thread ids

  my_item_t *head; // head of queue
  my_item_t *tail; // tail of queue

  int queue_max_num; // max number of queue
  int queue_cur_num; // current number of queue

  pthread_mutex_t mutex;          // to lock the src access
  pthread_cond_t queue_is_full;   // queue is full
  pthread_cond_t queue_not_full;  // queue not full
  pthread_cond_t queue_is_empty;  // queue is empty
  pthread_cond_t queue_not_empty; // queue not empty
} my_queue_t;

void async_init(int);
void async_run(void (*fx)(int), int args);

#endif
