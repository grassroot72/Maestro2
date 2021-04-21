/* Copyright (c) 2015, Chao Wang <hit9@icloud.com>
 * Copyright (c) 2021 Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "xmalloc.h"
#include "skiplist.h"

#define DEBUG
#include "debug.h"


/* Get random level between 1 and level_max. */
static int _rand_level(void)
{
  int level = 1;
  while ((random() & 0xffff) < (SKIPLIST_FACTOR_P * 0xffff)) {
    level++;
  }
  if (level < SKIPLIST_LEVEL_MAX) return level;
  return SKIPLIST_LEVEL_MAX;
}

/* create skipnode */
static skipnode_t *_skipnode_new(int level,
                                 void *data)
{
  skipnode_t *node = xmalloc(sizeof(skipnode_t));
  if (node != NULL) {
    node->data = data;
    node->prev = NULL;
    node->next = xmalloc(level * sizeof(skipnode_t *));
    if (node->next == NULL) {
      xfree(node);
      return NULL;
    }

    int i;
    for (i = 0; i < level; i++) node->next[i] = NULL;
  }
  return node;
}

/* delete skipnode */
static void _skipnode_delete(skipnode_t *node)
{
  if (node != NULL) {
    if (node->next != NULL) xfree(node->next);
    xfree(node);
  }
}

/* create skiplist */
skiplist_t *skiplist_new(skip_cmp_fn cmp,
                         skip_del_fn del,
                         skip_prt_fn prt)
{
  skiplist_t *skiplist = xmalloc(sizeof(skiplist_t));

  if (skiplist != NULL) {
    skiplist->size = 0;
    skiplist->level = 1;
    skiplist->cmp = cmp;
    skiplist->del = del;
    skiplist->prt = prt;
    skiplist->head = _skipnode_new(SKIPLIST_LEVEL_MAX, NULL);
    if (skiplist->head == NULL) {
      xfree(skiplist);
      return NULL;
    }
    skiplist->tail = skiplist->head;
    pthread_mutex_init(&skiplist->mutex, NULL);
  }
  return skiplist;
}

/* delete skiplist. */
void skiplist_delete(skiplist_t *skiplist)
{
  if (skiplist != NULL) {
    skiplist_clear(skiplist);
    if (skiplist->head != NULL) _skipnode_delete(skiplist->head);
    xfree(skiplist);
  }
}

void skiplist_set_cmp(skiplist_t *skiplist,
                      skip_cmp_fn cmp)
{
  skiplist->cmp = cmp;
}

void skiplist_set_del(skiplist_t *skiplist,
                      skip_del_fn del)
{
  skiplist->del = del;
}

/* clear skiplist */
void skiplist_clear(skiplist_t *skiplist)
{
  while (skiplist->size != 0) {
    void *data = skiplist_popfirst(skiplist);
    if (skiplist->del) skiplist->del(data);
  }
}

/* get data by score, NULL on not found. */
void *skiplist_search(skiplist_t *skiplist,
                      void *data)
{
  skipnode_t *node = skiplist->head;
  int i;
  for (i = skiplist->level - 1; i >= 0; i--) {
    while (node->next[i] != NULL) {
      if (skiplist->cmp(node->next[i]->data, data) < 0)
        node = node->next[i];
      else break;
    }
  }

  node = node->next[0];
  if (node != NULL && skiplist->cmp(node->data, data) == 0)
    return node->data;
  /* not found */
  return NULL;
}

/* insert data to skiplist */
int skiplist_insert(skiplist_t *skiplist,
                    void *data)
{
  skipnode_t *update[SKIPLIST_LEVEL_MAX];
  skipnode_t *node = skiplist->head;
  int i;
  for (i = skiplist->level - 1; i >= 0; i--) {
    while (node->next[i] != NULL) {
      if (skiplist->cmp(node->next[i]->data, data) < 0)
        node = node->next[i];
      else break;
    }
    update[i] = node;
  }

  /* same key found, skip */
  if (node->next[0] != NULL && skiplist->cmp(node->next[0]->data, data) == 0)
    return 0;

  int level = _rand_level();
  if (level > skiplist->level) {
    for (i = skiplist->level; i < level; i++) update[i] = skiplist->head;
    skiplist->level = level;
  }

  node = _skipnode_new(level, data);
  if (node == NULL) return 0;

  for (i = 0; i < level; i++) {
    node->next[i] = update[i]->next[i];
    update[i]->next[i] = node;
  }

  node->prev = update[0];
  if (node->next[0] == NULL)
    skiplist->tail = node;
  else
    node->next[0]->prev = node;

  skiplist->size++;
  return 1;
}

/* remove node by score (the first target), NULL on not found. */
int skiplist_remove(skiplist_t *skiplist,
                    void *data)
{
  skipnode_t *update[SKIPLIST_LEVEL_MAX];
  skipnode_t *head = skiplist->head;
  skipnode_t *node = head;
  int i;
  for (i = skiplist->level - 1; i >= 0; i--) {
    while (node->next[i] != NULL) {
      if (skiplist->cmp(node->next[i]->data, data) < 0)
        node = node->next[i];
      else break;
    }
    update[i] = node;
  }

  node = node->next[0];
  if (node == NULL || skiplist->cmp(node->data, data) != 0)
    /* not found */
    return 0;

  for (i = 0; i < skiplist->level; i++) {
    if (update[i]->next[i] == node)
        update[i]->next[i] = node->next[i];
  }

  while (skiplist->level > 1 && head->next[skiplist->level - 1] == NULL) {
    skiplist->level--;
  }

  if (node->next[0] != NULL) node->next[0]->prev = node->prev;
  if (node == skiplist->tail) skiplist->tail = node->prev;

  if (skiplist->del) skiplist->del(node->data);
  _skipnode_delete(node);
  skiplist->size--;
  return 1;
}

/* pop the first node and get its data, NULL on empty.
 * We assert that only head can establish forward link
 * to the first node. */
void *skiplist_popfirst(skiplist_t *skiplist)
{
  if (skiplist->size == 0) return NULL;

  skipnode_t *head = skiplist->head;
  skipnode_t *node = head->next[0];
  int i;
  for (i = 0; i < skiplist->level; i++) {
    if (head->next[i] == node) head->next[i] = node->next[i];
  }

  while (skiplist->level > 1 && head->next[skiplist->level - 1] == NULL) {
    skiplist->level--;
  }

  if (node->next[0] != NULL) node->next[0]->prev = node->prev;
  if (node == skiplist->tail) skiplist->tail = node->prev;

  void *data = node->data;
  _skipnode_delete(node);
  skiplist->size--;
  return data;
}

/* pop the last node and get its data, NULL on empty.
 * We assert that tail's forwards links */
void *skiplist_poplast(skiplist_t *skiplist)
{
  skipnode_t *update[SKIPLIST_LEVEL_MAX];
  skipnode_t *head = skiplist->head;
  skipnode_t *tail = skiplist->tail;
  skipnode_t *node = head;
  int i;
  for (i = skiplist->level - 1; i >= 0; i--) {
    while (node->next[i] != NULL && node->next[i] != tail) {
      node = node->next[i];
    }
    update[i] = node;
  }

  for (i = 0; i < skiplist->level; i++) {
    if (update[i]->next[i] == tail) update[i]->next[i] = NULL;
  }

  while (skiplist->level > 1 && head->next[skiplist->level - 1] == NULL) {
    skiplist->level -= 1;
  }

  void *data = tail->data;
  skiplist->tail = tail->prev;
  _skipnode_delete(tail);
  skiplist->size--;
  return data;
}

/* get first node, NULL on empty */
skipnode_t *skiplist_first(skiplist_t *skiplist)
{
  if (skiplist->size == 0) return NULL;
  return skiplist->head->next[0];
}

/* get last node, NULL on empty */
skipnode_t *skiplist_last(skiplist_t *skiplist)
{
  if (skiplist->size == 0) return NULL;
  return skiplist->tail;
}

/* create a skiplist iterator */
skiplist_iter_t *skiplist_iter_new(skiplist_t *skiplist)
{
  skiplist_iter_t *iter = xmalloc(sizeof(skiplist_iter_t));

  if (iter != NULL) {
    iter->skiplist = skiplist;
    iter->node = skiplist->head;
  }
  return iter;
}

/* Free skiplist iter. */
void skiplist_iter_delete(skiplist_iter_t *iter)
{
  if (iter != NULL) xfree(iter);
}

/* seek skiplist next, NULL on end. */
skipnode_t *skiplist_iter_next(skiplist_iter_t *iter)
{
  iter->node = iter->node->next[0];
  return iter->node;
}

/* seek skiplist prev, NULL on end. */
skipnode_t *skiplist_iter_prev(skiplist_iter_t *iter)
{
  iter->node = iter->node->prev;
  if (iter->node == iter->skiplist->head) return NULL;
  return iter->node;
}

/* rewind skiplist iterator. */
void skiplist_iter_rewind(skiplist_iter_t *iter)
{
  iter->node = iter->skiplist->head;
}

/* print the skiplist schema. */
void skiplist_print(skiplist_t *skiplist)
{
  skipnode_t *node;
  int i;
  for (i = 0; i < skiplist->level; i++) {
    node = skiplist->head->next[i];
    D_PRINT("Level[%d]:\n", i);
    while (node != NULL) {
      skiplist->prt(node->data);
      node = node->next[i];
    }
  }
}
