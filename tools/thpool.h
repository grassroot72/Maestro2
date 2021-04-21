/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#ifndef _THPOOL_H_
#define _THPOOL_H_


#define THREAD_POOL_MAX_THREADS 64
#define THREAD_POOL_MIN_THREADS 4

#define THREAD_STATE_BUSY 0x1
#define THREAD_STATE_IDLE 0x2

#define THREAD_STOPPING 0x1
#define THREAD_RUNNING 0x2

#define THREAD_IDLE_LEVEL 0.2
#define THREAD_BUSY_LEVEL 0.8

#define MASTER_INTERVAL 30


typedef struct _th_dlnode {
  struct _th_dlnode *next;
  struct _th_dlnode *prev;
} th_dlnode_t;

typedef struct {
  void (*routine)(void *arg);
  void *arg;
  th_dlnode_t entry;
} th_task_t;

typedef struct {
  pthread_t tid;
  pthread_mutex_t mutex;    /* mutex lock for task queue */
  pthread_cond_t cond;
  int state;                /* THREAD_STATE_BUSY or THREAD_STATE_IDLE */
  int stop;                 /* mark if the thread is going to quit */
  int queue_size;           /* the number of tasks in the task_queue */
  th_dlnode_t task_queue;   /* the head of task queue*/
  th_dlnode_t worker_entry; /* a node in the thread pool's worker queue */
  th_dlnode_t idle_entry;   /* a node in the thread pool's idle queue */
  void *tp;
} thread_t;

typedef struct {
  th_dlnode_t worker_queue; /*the head of worker queue */
  th_dlnode_t idle_queue;   /*the head of idle queue */
  thread_t *master_thread;  /*the struct of master thread */
  pthread_mutex_t global;   /*the global lock for thread pool */
  thread_t *task_next;
  int size;                /* the number of threads */
  int max_size;            /* maxium number of threads */
  int min_size;            /* minium numbers of threads */
  double low_level;        /* the maxium proportion threshold of idle threads */
  double high_level;       /* the minium proportion threshold of idle threads */
  int master_interval;     /* the working interval of master thread */
} thpool_t;


thpool_t *thpool_new(int size);

void thpool_delete(thpool_t *p);

void thpool_add_task(thpool_t *p,
                     void (*routine)(void *),
                     void *arg);


#endif
