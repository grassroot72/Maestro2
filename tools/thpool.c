/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "xmalloc.h"
#include "thpool.h"

#define DEBUG
#include "debug.h"


#define _offsetof(type, member) ((size_t)&(((type *)0)->member))

#define container_of(ptr, type, member) ({                   \
          const typeof(((type *)0)->member) *_mptr = (ptr);  \
          (type *)((char *)_mptr - _offsetof(type, member)); \
        })

#define list_entry(list_ptr, type, member) \
        container_of(list_ptr, type, member)

#define list_first_entry(list_ptr, type, member) \
        list_entry(list_ptr->next, type, member)

#define list_next_entry(elem_ptr, member) \
        list_entry((elem_ptr)->member.next, typeof(*(elem_ptr)), member)

#define list_prev_entry(elem_ptr, member) \
        list_entry((elem_ptr)->member.prev, typeof(*(elem_ptr)), member)

#define list_foreach_entry(elem_ptr, head, member)                         \
        for (elem_ptr = list_first_entry(head, typeof(*elem_ptr), member); \
             &(elem_ptr->member) != (head);                                \
             elem_ptr = list_next_entry(elem_ptr, member))


static void list_set_head(th_dlnode_t *list)
{
  list->next = list;
  list->prev = list;
}

static void _list_add(th_dlnode_t *node,
                      th_dlnode_t *prev,
                      th_dlnode_t *next)
{
  node->next = next;
  node->prev = prev;
  prev->next = node;
  next->prev = node;
}

static void list_add_tail(th_dlnode_t *node,
                          th_dlnode_t *head)
{
  _list_add(node, head->prev, head);
}

static void _list_del(th_dlnode_t *prev,
                      th_dlnode_t *next)
{
  prev->next = next;
  next->prev = prev;
}

static void list_del(th_dlnode_t *entry)
{
  _list_del(entry->prev, entry->next);
}


static void *_worker_cb(void *arg)
{
  thread_t *curr = (thread_t *)arg;
  thpool_t *tp = (thpool_t *)(curr->tp);
  th_task_t *t;

  for (;;) {
    pthread_mutex_lock(&curr->mutex);
    while (curr->queue_size == 0 && curr->stop != THREAD_STOPPING) {
      pthread_cond_wait(&curr->cond, &curr->mutex);
    }

    if (curr->stop == THREAD_STOPPING) {
      if (curr->queue_size == 0) {
        pthread_mutex_unlock(&curr->mutex);
        pthread_mutex_lock(&tp->global);
        list_del(&curr->worker_entry);
        list_del(&curr->idle_entry);
        tp->size--;
        pthread_mutex_unlock(&tp->global);
        xfree(curr);
        break;
      }
      else {
        pthread_mutex_unlock(&curr->mutex);
        continue;
      }
    }
    else {
      t = list_entry(curr->task_queue.next, th_task_t, entry);
      if (t) t->routine(t->arg);
      list_del(&t->entry);
      xfree(t);
      curr->queue_size--;
    }

    if (curr->queue_size == 0) {
      pthread_mutex_lock(&tp->global);
      list_add_tail(&(curr->idle_entry), &(tp->idle_queue));
      pthread_mutex_unlock(&tp->global);
      curr->state = THREAD_STATE_IDLE;
    }
    pthread_mutex_unlock(&curr->mutex);
  }
  return NULL;
}

static int _thread_add(thpool_t *p,
                       thread_t *t)
{
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  int ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (ret != 0) {
    pthread_attr_destroy(&attr);
    return -1;
  }

  list_set_head(&t->task_queue);
  pthread_mutex_lock(&p->global);
  list_add_tail(&(t->worker_entry), &(p->worker_queue));
  list_add_tail(&(t->idle_entry), &(p->idle_queue));
  p->size++;
  pthread_mutex_unlock(&p->global);
  t->state = THREAD_STATE_IDLE;
  t->state = THREAD_RUNNING;
  t->queue_size = 0;
  t->tp = (void *)p;
  pthread_mutex_init(&t->mutex, NULL);
  pthread_cond_init(&t->cond, NULL);

  pthread_create(&t->tid, &attr, _worker_cb, (void *)t);
  pthread_attr_destroy(&attr);
  return 0;
}

static void *_master_cb(void *arg)
{
  thpool_t *p = (thpool_t *)arg;
  thread_t *t;

  sleep(p->master_interval);
  for (;;) {
    int busy = 0;
    int idle = 0;

    pthread_mutex_lock(&p->global);
    list_foreach_entry(t, (&(p->worker_queue)), worker_entry) {
      if (t->state == THREAD_STATE_IDLE)
        idle++;
      else if (t->state == THREAD_STATE_BUSY)
        busy++;
    }

    double threshold = busy / p->size;
    //D_PRINT("[POOL] threshold:%.2lf low_level:%.2lf\n",
    //        threshold, p->low_level);
    if (threshold < p->low_level) {
      th_dlnode_t *next;
      int delete_num = p->size * (p->low_level - threshold);
      while (delete_num--) {
        next = p->idle_queue.next;
        thread_t *t = list_entry(next, thread_t, idle_entry);
        /* stop a thread */
        pthread_mutex_lock(&t->mutex);
        t->stop = THREAD_STOPPING;
        pthread_cond_signal(&t->cond);
        pthread_mutex_unlock(&t->mutex);
        next = next->next;
        D_PRINT("[POOL] stopping a thread...\n");
      }
    }
    if (threshold > p->high_level) {
      int add_num = p->size * (p->high_level - threshold);
      while (add_num--) {
        thread_t *t = xmalloc(sizeof(thread_t));
        if (t == NULL)
          break;
        else
          _thread_add(p, t);
      }
    }

    pthread_mutex_unlock(&p->global);
    D_PRINT("[POOL] busy:%d, idle:%d\n", busy, idle);
    sleep(p->master_interval);
  }
  return NULL;
}

static int _pool_init(thpool_t *p,
                      double low_level,
                      double high_level,
                      int master_interval)
{
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  int ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (ret != 0) {
    pthread_attr_destroy(&attr);
    return -1;
  }

  pthread_mutex_init(&p->global, NULL);
  p->max_size = THREAD_POOL_MAX_THREADS;
  p->min_size = THREAD_POOL_MIN_THREADS;
  p->low_level = low_level;
  p->high_level = high_level;
  p->master_interval = master_interval;
  list_set_head(&p->worker_queue);
  list_set_head(&p->idle_queue);

  int size = p->size;
  int i;
  thread_t *t = NULL;
  p->size = 0;
  for (i = 0; i < size; i++) {
    t = xmalloc(sizeof(thread_t));
    if (t == NULL) return -1;
    if (_thread_add(p, t) == -1) return -1;
  }
  pthread_create(&p->master_thread->tid, &attr, _master_cb, (void *)p);
  pthread_attr_destroy(&attr);
  p->task_next = list_first_entry((&p->worker_queue), thread_t, worker_entry);
  return 0;
}

thpool_t *thpool_new(int size)
{
  thpool_t *p = xmalloc(sizeof(thpool_t));
  if (p == NULL) return NULL;

  thread_t *master_thread = xmalloc(sizeof(thread_t));
  if (master_thread == NULL) {
    xfree(p);
    return NULL;
  }
  p->master_thread = master_thread;
  p->size = size;

  _pool_init(p, THREAD_IDLE_LEVEL, THREAD_BUSY_LEVEL, MASTER_INTERVAL);
  return p;
}

void thpool_delete(thpool_t *p)
{
  thread_t *th;
  th_task_t *t;
  list_foreach_entry(th, (&(p->worker_queue)), worker_entry) {
    while (th->queue_size) {
      t = list_entry(th->task_queue.next, th_task_t, entry);
      list_del(&t->entry);
      xfree(t);
      th->queue_size--;
    }
    xfree(th);
  }
  xfree(p->master_thread);
  xfree(p);
}

void thpool_add_task(thpool_t *p,
                     void (*routine)(void *),
                     void *arg)
{
  thread_t *th = NULL;
  thread_t *last = NULL;

  th_task_t *t = xmalloc(sizeof(th_task_t));
  if (t == NULL) return;
  t->routine = routine;
  t->arg = arg;

  pthread_mutex_lock(&p->global);
  th = p->task_next;
  last = list_entry(p->worker_queue.prev, thread_t, worker_entry);
  if (th == last)
    p->task_next = list_first_entry((&p->worker_queue), thread_t, worker_entry);
  else
    p->task_next = list_next_entry(th, worker_entry);
  pthread_mutex_unlock(&p->global);

  pthread_mutex_lock(&th->mutex);
  list_add_tail(&(t->entry), &(th->task_queue));
  th->queue_size++;
  if (th->queue_size == 1) {
    th->state = THREAD_STATE_BUSY;
    pthread_mutex_lock(&p->global);
    list_del(&(th->idle_entry));
    pthread_mutex_unlock(&p->global);
  }

  pthread_cond_signal(&th->cond);
  pthread_mutex_unlock(&th->mutex);
}
