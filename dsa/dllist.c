/* MIT license
 * Copyright (c) 2010 TJ Holowaychuk <tj@vision-media.ca>
 * Copyright (c) 2021 Edward LEI <edward_lei72@hotmail.com> */

#include <stdlib.h>
#include "xmalloc.h"
#include "dllist.h"


/* Allocates a new list_node_t. NULL on failure. */
static dlnode_t *_node_new(void *data)
{
  dlnode_t *n = xmalloc(sizeof(dlnode_t));
  if (!n) return NULL;
  n->prev = NULL;
  n->next = NULL;
  n->data = data;
  return n;
}


/* Allocate a new list_t. NULL on failure. */
dllist_t *dll_new(dll_cmp_fn cmp,
                  dll_del_fn del,
                  dll_prt_fn prt)
{
  dllist_t *list = xmalloc(sizeof(dllist_t));
  if (!list) return NULL;
  list->head = NULL;
  list->tail = NULL;
  list->cmp = cmp;
  list->del = del;

  list->size = 0;
  return list;
}

void dll_delete(dllist_t *list)
{
  size_t size = list->size;
  dlnode_t *next;
  dlnode_t *curr = list->head;

  while (size--) {
    next = curr->next;
    if (list->del) list->del(curr->data);
    xfree(curr);
    curr = next;
  }
  xfree(list);
}

/*
 *        tail --+ +-------+
 *               V |       V     push
 * ... o-><-o-><-o-> ... <-o->|  <<<-
 *               ^       |
 *               +-------+
 *
 * Append to the list */
int dll_rpush(dllist_t *list,
              void *data)
{
  dlnode_t *n = _node_new(data);
  if (n == NULL) return 0;

  if (list->size) {
    n->prev = list->tail;
    n->next = NULL;
    list->tail->next = n;
    list->tail = n;
  }
  else {
    list->head = list->tail = n;
    n->prev = n->next = NULL;
  }

  ++list->size;
  return 1;
}

/*
 *                          +-- tail
 *                     pop  V
 * ... o-><o-><-o->|  ->>>  |<-o->|
 *              ^              |
 *              +--------------+
 *
 * detach the last node in the list */
int dll_rpop(dllist_t *list)
{
  if (!list->size) return 0;
  dlnode_t *n = list->tail;

  if (--list->size)
    (list->tail = n->prev)->next = NULL;
  else
    list->tail = list->head = NULL;

  n->next = n->prev = NULL;
  if (list->del) list->del(n->data);
  xfree(n);
  return 1;
}

/*
 *          +-------+
 * push     |       V
 * ->>>  |<-o-> ... <-o-><-o-><-o ...
 *        ^       | ^
 *        +-------+ +-- head
 *
 * Prepend to the list */
int dll_lpush(dllist_t *list,
              void *data)
{
  dlnode_t *n = _node_new(data);
  if (n == NULL) return 0;

  if (list->size) {
    n->next = list->head;
    n->prev = NULL;
    list->head->prev = n;
    list->head = n;
  }
  else {
    list->head = list->tail = n;
    n->prev = n->next = NULL;
  }

  ++list->size;
  return 1;
}

/*
 *    +-- head
 *    V     pop
 * |<-o->|  <<<-  |<-o-><-o-><-o ...
 *    ^              |
 *    +--------------+
 *
 * detach the first node in the list */
int dll_lpop(dllist_t *list)
{
  if (!list->size) return 0;
  dlnode_t *n = list->head;

  if (--list->size)
    (list->head = n->next)->prev = NULL;
  else
    list->head = list->tail = NULL;

  n->next = n->prev = NULL;
  if (list->del) list->del(n->data);
  xfree(n);
  return 1;
}

/* Return the found data or NULL. */
void *dll_search(dllist_t *list,
                 void *data)
{
  dll_iter_t *it = dll_iter_new(list, DLLIST_HEAD);
  dlnode_t *n;

  while ((n = dll_iter_next(it))) {
    if (list->cmp) {
      if (list->cmp(data, n->data)) {
        dll_iter_delete(it);
        return n->data;
      }
    }
    else {
      if (data == n->data) {
        dll_iter_delete(it);
        return n->data;
      }
    }
  }

  dll_iter_delete(it);
  return NULL;
}

/* Return the data at the given index or NULL. */
void *dll_at(dllist_t *list,
             int index)
{
  dll_dir_t direction = DLLIST_HEAD;

  if (index < 0) {
    direction = DLLIST_TAIL;
    index = ~index;
  }

  if ((unsigned)index < list->size) {
    dll_iter_t *it = dll_iter_new(list, direction);
    dlnode_t *n = dll_iter_next(it);
    while (index--) n = dll_iter_next(it);
    dll_iter_delete(it);
    return n->data;
  }
  return NULL;
}

/*
 *               +-----------+
 *               V           |
 * ... o-><-o-><-o->  <-x->  <-o-><-o-><-o ...
 *                 |           ^
 *                 +-----------+
 *
 * Remove the given node from the list,
 * freeing it and it's value */
void dll_remove(dllist_t *list,
                dlnode_t *node)
{
  node->prev
    ? (node->prev->next = node->next)
    : (list->head = node->next);

  node->next
    ? (node->next->prev = node->prev)
    : (list->tail = node->prev);

  if (list->del) list->del(node->data);
  xfree(node);
  --list->size;
}


/* Allocate a new list_iterator_t. NULL on failure.
 * Accepts a direction, which may be LIST_HEAD or LIST_TAIL. */
dll_iter_t *dll_iter_new(dllist_t *list,
                         dll_dir_t direction)
{
  dlnode_t *node = (direction == DLLIST_HEAD)
    ? list->head
    : list->tail;
  return dll_iter_new_from_node(node, direction);
}

/* Allocate a new list_iterator_t with the given start
 * node. NULL on failure. */
dll_iter_t *dll_iter_new_from_node(dlnode_t *node,
                                   dll_dir_t direction)
{
  dll_iter_t *iter = xmalloc(sizeof(dll_iter_t));
  if (!iter) return NULL;
  iter->next = node;
  iter->direction = direction;
  return iter;
}

/* Return the next list_node_t or NULL when no more
 * nodes remain in the list. */
dlnode_t *dll_iter_next(dll_iter_t *iter)
{
  dlnode_t *curr = iter->next;
  if (curr) {
    iter->next = (iter->direction == DLLIST_HEAD)
      ? curr->next
      : curr->prev;
  }
  return curr;
}

/* Free the list iterator. */
void dll_iter_delete(dll_iter_t *iter)
{
  xfree(iter);
  iter = NULL;
}
