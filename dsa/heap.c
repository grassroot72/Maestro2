/* license: BSD license
 * Copyright Willem
 * https://github.com/willemt/heap
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "xmalloc.h"
#include "heap.h"


size_t heap_sizeof(unsigned int size)
{
  return sizeof(heap_t) + size * sizeof(void *);
}

static int _child_left(const int idx)
{
  return idx * 2 + 1;
}

static int _child_right(const int idx)
{
  return idx * 2 + 2;
}

static int _parent(const int idx)
{
  return (idx - 1) / 2;
}

void heap_init(heap_t* h,
               hp_cmp_fn cmp,
               hp_del_fn del,
               hp_prt_fn prt,
               unsigned int size)
{
  h->cmp = cmp;
  h->del = del;
  h->prt = prt;
  h->size = size;
  h->count = 0;
}

heap_t *heap_new(hp_cmp_fn cmp,
                 hp_del_fn del,
                 hp_prt_fn prt)
{
  heap_t *h = xmalloc(heap_sizeof(DEFAULT_CAPACITY));
  if (!h) return NULL;

  heap_init(h, cmp, del, prt, DEFAULT_CAPACITY);
  pthread_mutex_init(&h->mutex, NULL);
  return h;
}

void heap_delete(heap_t *h)
{
  if (h->del) {
    int i;
    for (i = 0; i < h->count; i++) {
      h->del(h->array[i]);
    }
  }
  xfree(h);
}

static heap_t *_ensurecapacity(heap_t *h)
{
  if (h->count < h->size) return h;
  h->size += DEFAULT_CAPACITY;
  return xrealloc(h, heap_sizeof(h->size));
}

static void _swap(heap_t *h,
                  const int i1,
                  const int i2)
{
  void *tmp = h->array[i1];

  h->array[i1] = h->array[i2];
  h->array[i2] = tmp;
}

static int _pushup(heap_t *h,
                   unsigned int idx)
{
  /* 0 is the root node */
  while (0 != idx) {
    int parent = _parent(idx);

    /* we are smaller than the parent */
    if (h->cmp(h->array[idx], h->array[parent]) < 0)
      return -1;
    else
      _swap(h, idx, parent);

    idx = parent;
  }
  return idx;
}

static void _pushdown(heap_t *h,
                      unsigned int idx)
{
  while (1) {
    unsigned int childl, childr, child;

    childl = _child_left(idx);
    childr = _child_right(idx);

    if (childr >= h->count) {
      /* can't pushdown any further */
      if (childl >= h->count) return;
      child = childl;
    }
    /* find biggest child */
    else if (h->cmp(h->array[childl], h->array[childr]) < 0)
      child = childr;
    else
      child = childl;

    /* idx is smaller than child */
    if (h->cmp(h->array[idx], h->array[child]) < 0) {
      _swap(h, idx, child);
      idx = child;
      /* bigger than the biggest child, we stop, we win */
    }
    else
      return;
  }
}

static void _heap_offerx(heap_t *h,
                         void *item)
{
  h->array[h->count] = item;
  /* ensure heap properties */
  _pushup(h, h->count++);
}

int heap_offerx(heap_t *h, void *item)
{
  if (h->count == h->size) return -1;

  _heap_offerx(h, item);
  return 0;
}

int heap_offer(heap_t **h,
               void *item)
{
  if (0 == (*h = _ensurecapacity(*h)))
    return -1;
  _heap_offerx(*h, item);
  return 0;
}

void *heap_poll(heap_t *h)
{
  if (0 == heap_count(h)) return NULL;

  void *item = h->array[0];
  h->array[0] = h->array[h->count - 1];
  h->count--;

  if (h->count > 1) _pushdown(h, 0);
  return item;
}

void *heap_peek(const heap_t *h)
{
  if (0 == heap_count(h)) return NULL;
  return h->array[0];
}

void heap_clear(heap_t *h)
{
  h->count = 0;
}

static int _item_get_idx(const heap_t *h,
                         const void *item)
{
  unsigned int idx;

  for (idx = 0; idx < h->count; idx++) {
    if (0 == h->cmp(h->array[idx], item))
      return idx;
  }
  return -1;
}

void *heap_remove_item(heap_t *h,
                       const void *item)
{
  int idx = _item_get_idx(h, item);
  if (idx == -1) return NULL;

  /* swap the item we found with the last item on the heap */
  void *ret_item = h->array[idx];
  h->array[idx] = h->array[h->count - 1];
  h->array[h->count - 1] = 0;
  h->count -= 1;

  /* ensure heap property */
  _pushup(h, idx);
  return ret_item;
}

int heap_contains_item(const heap_t *h,
                       const void *item)
{
  return _item_get_idx(h, item) != -1;
}

int heap_count(const heap_t *h)
{
  return h->count;
}

int heap_size(const heap_t *h)
{
  return h->size;
}
