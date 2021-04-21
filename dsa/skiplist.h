/* Copyright (c) 2015, Chao Wang <hit9@icloud.com>
 * Copyright (c) 2021 Edward LEI <edward_lei72@hotmail.com> */

#ifndef _SKIPLIST_H_
#define _SKIPLIST_H_


#define SKIPLIST_LEVEL_MAX 32 /* max skiplist level (0~11) */
#define SKIPLIST_FACTOR_P 0.25

#define skiplist_foreach(sl, node)                          \
         for ((node) = (sl)->head->next[0]; (node) != NULL; \
         (node) = (node)->next[0])

typedef int (*skip_cmp_fn)(const void *p1, const void *p2);
typedef void (*skip_del_fn)(void *p);
typedef void (*skip_prt_fn)(const void *p);

typedef struct _skipnode {
  void *data;              /* node data */
  struct _skipnode **next; /* node forward links */
  struct _skipnode *prev;  /* node backward link */
} skipnode_t;

typedef struct {
  size_t size;       /* skiplist size */
  int level;         /* skiplist level */
  skipnode_t *head;  /* skiplist head */
  skipnode_t *tail;  /* skiplist tail */
  skip_cmp_fn cmp;   /* comparator */
  skip_del_fn del;   /* deleter */
  skip_prt_fn prt;   /* print data */

  pthread_mutex_t mutex;
} skiplist_t;

typedef struct {
  skiplist_t *skiplist;  /* skiplist to iterate */
  skipnode_t *node;      /* current skiplist node */
} skiplist_iter_t;


skiplist_t *skiplist_new(skip_cmp_fn cmp,
                         skip_del_fn del,
                         skip_prt_fn prt);

void skiplist_delete(skiplist_t *skiplist);

void skiplist_set_cmp(skiplist_t *skiplist,
                      skip_cmp_fn cmp);

void skiplist_set_del(skiplist_t *skiplist,
                      skip_del_fn del);

void skiplist_clear(skiplist_t *skiplist);

void *skiplist_search(skiplist_t *skiplist,
                      void *data);

int skiplist_insert(skiplist_t *skiplist,
                    void *data);

int skiplist_remove(skiplist_t *skiplist,
                    void *data);

void *skiplist_popfirst(skiplist_t *skiplist);

void *skiplist_poplast(skiplist_t *skiplist);

skipnode_t *skiplist_first(skiplist_t *skiplist);

skipnode_t *skiplist_last(skiplist_t *skiplist);


skiplist_iter_t *skiplist_iter_new(skiplist_t *skiplist);

void skiplist_iter_delete(skiplist_iter_t *iter);

skipnode_t *skiplist_iter_next(skiplist_iter_t *iter);

skipnode_t *skiplist_iter_prev(skiplist_iter_t *iter);

void skiplist_iter_rewind(skiplist_iter_t *iter);

void skiplist_print(skiplist_t *skiplist);


#endif
