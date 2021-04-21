/* license: MIT license
 * Copyright 2020 Alan Tseng
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include "xmalloc.h"
#include "sllist.h"

//#define DEBUG
#include "debug.h"


static slnode_t *_node_new(void *data)
{
  slnode_t *n = xmalloc(sizeof(slnode_t));
  if (n == NULL) return NULL;
  n->data = data;
  return n;
}

sllist_t *sll_new(sll_cmp_fn cmp,
                  sll_del_fn del,
                  sll_prt_fn prt)
{
  sllist_t *list = xmalloc(sizeof(sllist_t));
  if (list == NULL) return NULL;
  list->head = NULL;
  list->tail = NULL;
  list->cmp = cmp;
  list->del = del;
  list->prt = prt;
  return list;
}

void sll_delete(sllist_t *list)
{
  slnode_t *n;
  while (list->head != NULL) {
    n = list->head;
    if (list->del) list->del(n->data);
    list->head = n->next;
    xfree(n);
  }
  D_PRINT("[SLL] nil\n");
  xfree(list);
}

int sll_lpush(sllist_t *list,
              void *data)
{
  slnode_t *n = _node_new(data);
  if (n == NULL) return 0;
  n->next = list->head;
  if (sll_empty(list)) list->tail = n;
  list->head = n;
  return 1;
}

int sll_lpop(sllist_t *list)
{
  if (sll_empty(list)) return 0;
  slnode_t *n = list->head;
  list->head = n->next;
  if (sll_empty(list)) list->tail = NULL;
  if (list->del) list->del(n->data);
  xfree(n);
  return 1;
}

int sll_rpush(sllist_t *list,
              void *data)
{
  if (sll_empty(list))
    return sll_lpush(list, data);
  else
    return sll_insert(list, list->tail, data);
}

int sll_rpop(sllist_t *list)
{
  if (sll_empty(list)) return 0;
  slnode_t *n = list->head;
  while (n->next != list->tail) n = n->next;
  n->next = NULL;
  if (list->del) list->del(list->tail->data);
  xfree(list->tail);
  list->tail = n;
  return 1;
}

int sll_empty(sllist_t *list)
{
  return list->head == NULL;
}

void *sll_lpeek(sllist_t *list)
{
  if (sll_empty(list)) return NULL;
  return list->head->data;
}

void *sll_rpeek(sllist_t *list)
{
  if (sll_empty(list)) return NULL;
  return list->tail->data;
}

void *sll_search(sllist_t *list,
                 void *data)
{
  slnode_t *n = list->head;
  int cmp;
  while (n != NULL) {
    cmp = list->cmp(n->data, data);
    if (cmp == 0) return n->data;
    n = n->next;
  }
  return NULL;
}

int sll_insert(sllist_t *list,
               slnode_t *node,
               void *data)
{
  slnode_t *n = _node_new(data);
  if (n == NULL) return 0;

  slnode_t *nxt = node->next;
  node->next = n;
  n->next = nxt;

  if (n->next == NULL) list->tail = n;
  return 1;
}

int sll_remove(sllist_t *list,
               slnode_t *node)
{
  if (node->next == NULL) return 0;
  /* the order is node, n, nxt */
  slnode_t *n = node->next;
  slnode_t *nxt = n->next;
  node->next = nxt;

  if (n->next == NULL) list->tail = node;
  list->del(n->data);
  xfree(n);
  return 1;
}

void sll_print(sllist_t *list)
{
  slnode_t *n = list->head;
  while (n != NULL) {
    list->prt(n->data);
    n = n->next;
  }
}
