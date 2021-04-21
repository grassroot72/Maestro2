/* license: MIT license
 * Copyright 2020 Alan Tseng
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#ifndef _SLLIST_H_
#define _SLLIST_H_


typedef int (*sll_cmp_fn) (const void *p1, const void *p2);
typedef void (*sll_del_fn) (void *data);
typedef void (*sll_prt_fn)(const void *p);


typedef struct _slnode {
  void *data;
  struct _slnode *next;
} slnode_t;

typedef struct {
  slnode_t *head;
  slnode_t *tail;

  sll_cmp_fn cmp;
  sll_del_fn del;
  sll_prt_fn prt;
} sllist_t;


/* singly linked list */
sllist_t *sll_new(sll_cmp_fn cmp,
                  sll_del_fn del,
                  sll_prt_fn prt);

void sll_delete(sllist_t *list);

int sll_lpush(sllist_t *list,
              void *data);

int sll_lpop(sllist_t *list);

int sll_rpush(sllist_t *list,
              void *data);

int sll_rpop(sllist_t *list);

int sll_empty(sllist_t *list);

void *sll_lpeek(sllist_t *list);

void *sll_rpeek(sllist_t *list);

void *sll_search(sllist_t *list,
                 void *data);

int sll_insert(sllist_t *list,
               slnode_t *node,
               void *data);

int sll_remove(sllist_t *list,
               slnode_t *node);

void sll_print(sllist_t *list);


#endif
