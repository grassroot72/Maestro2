/* MIT license
 * Copyright (c) 2010 TJ Holowaychuk <tj@vision-media.ca>
 * Copyright (c) 2021 Edward LEI <edward_lei72@hotmail.com> */

#ifndef _DLLIST_H_
#define _DLLIST_H_


typedef int (*dll_cmp_fn) (void *a, void *b);
typedef void (*dll_del_fn) (void *data);
typedef void (*dll_prt_fn)(const void *p);


/* list iterator direction */
typedef enum {
  DLLIST_HEAD,
  DLLIST_TAIL
} dll_dir_t;

/* node struct */
typedef struct _dlnode {
  void *data;
  struct _dlnode *prev;
  struct _dlnode *next;
} dlnode_t;

/* doubly linked list struct */
typedef struct {
  dlnode_t *head;
  dlnode_t *tail;

  dll_cmp_fn cmp;
  dll_del_fn del;
  dll_prt_fn prt;
  size_t size;
} dllist_t;

/* iterator struct */
typedef struct {
  dlnode_t *next;
  dll_dir_t direction;
} dll_iter_t;


/* doubly linked list */
dllist_t *dll_new(dll_cmp_fn cmp,
                  dll_del_fn del,
                  dll_prt_fn prt);

void dll_delete(dllist_t *list);

int dll_rpush(dllist_t *list,
              void *data);

int dll_rpop(dllist_t *list);

int dll_lpush(dllist_t *list,
              void *data);

int dll_lpop(dllist_t *list);

void *dll_search(dllist_t *list,
                 void *data);

void *dll_at(dllist_t *list,
             int index);

void dll_remove(dllist_t *list,
                dlnode_t *node);


/* iterator */
dll_iter_t *dll_iter_new(dllist_t *list,
                         dll_dir_t direction);

dll_iter_t *dll_iter_new_from_node(dlnode_t *node,
                                   dll_dir_t direction);

dlnode_t *dll_iter_next(dll_iter_t *iter);

void dll_iter_delete(dll_iter_t *iter);


#endif
