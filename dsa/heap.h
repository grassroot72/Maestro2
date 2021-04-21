/* license: BSD license
 * Copyright Willem
 * https://github.com/willemt/heap
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#ifndef _HEAP_H_
#define _HEAP_H_


#define DEFAULT_CAPACITY 512

typedef int (*hp_cmp_fn) (const void *p1, const void *p2);
typedef void (*hp_del_fn) (void *p);
typedef void (*hp_prt_fn)(const void *p);

typedef struct {
  pthread_mutex_t mutex;
  unsigned int size;   /* size of array */
  unsigned int count;  /* items within heap */
  hp_cmp_fn cmp;
  hp_del_fn del;
  hp_prt_fn prt;

  void *array[];
} heap_t;


heap_t *heap_new(hp_cmp_fn cmp,
                 hp_del_fn del,
                 hp_prt_fn prt);

void heap_init(heap_t* h,
               hp_cmp_fn cmp,
               hp_del_fn del,
               hp_prt_fn prt,
               unsigned int size);

void heap_delete(heap_t *hp);

int heap_offer(heap_t **h,
               void *item);

int heap_offerx(heap_t *h,
                void *item);

void *heap_poll(heap_t *h);

void *heap_peek(const heap_t *h);

void heap_clear(heap_t *h);

int heap_count(const heap_t *h);

int heap_size(const heap_t *h);

size_t heap_sizeof(unsigned int size);

void *heap_remove_item(heap_t *h,
                       const void *item);

int heap_contains_item(const heap_t *h,
                       const void *item);


#endif
