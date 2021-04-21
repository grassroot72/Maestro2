/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "xmalloc.h"
#include "util.h"
#include "rbtree.h"
#include "http_cache.h"

#define DEBUG
#include "debug.h"


#define MAX_CACHE_ITEMS 1024
#define MAX_CACHE_TIME 86400000    /* 24 x 60 x 60 = 1 day */


httpcache_t *httpcache_new()
{
  httpcache_t *data = xmalloc(sizeof(httpcache_t));
  return data;
}

void httpcache_set(httpcache_t *data,
                   char *path,
                   char *etag,
                   char *modified,
                   unsigned char *body,
                   const size_t len_body,
                   unsigned char *body_zipped,
                   const size_t len_zipped)
{
  data->path = path;
  data->etag = etag;
  data->last_modified = modified;
  data->stamp = mstime();
  data->body = body;
  data->len_body = len_body;
  data->body_zipped = body_zipped;
  data->len_zipped = len_zipped;
}

void httpcache_clear(void *data)
{
  if (data) {
    httpcache_t *cd = (httpcache_t *)data;
    if (cd->path) xfree(cd->path);
    if (cd->etag) xfree(cd->etag);
    if (cd->last_modified) xfree(cd->last_modified);
    if (cd->body) xfree(cd->body);
    if (cd->body_zipped) xfree(cd->body_zipped);
  }
}

void httpcache_delete(void *data)
{
  httpcache_clear(data);
  xfree(data);
  D_PRINT("[CACHE] deleted...\n");
}

int httpcache_compare(const void *curr,
                      const void *cache)
{
  char *s1 = ((httpcache_t *)curr)->path;
  char *s2 = ((httpcache_t *)cache)->path;
  return strcmp(s1, s2);
}

void httpcache_expire(void *arg)
{
  rbtree_t *cache = (rbtree_t *)arg;
  if (cache->size == 0) return;

  long curr_time = mstime();
  rbtrav_t *trav = rbtrav_new();
  void *data = rbtrav_first(trav, cache);
  httpcache_t *cd;

  do {
    if (data) {
      cd = (httpcache_t *)data;
      if (curr_time - cd->stamp >= MAX_CACHE_TIME) {
        if (pthread_mutex_trylock(&cache->mutex) == 0) {
          rbtree_remove(cache, data);
          pthread_mutex_unlock(&cache->mutex);
        }
      }
    }
    data = rbtrav_next(trav);
  } while (data);

  rbtrav_delete(trav);
}

void httpcache_print(const void *data)
{
  httpcache_t *p = (httpcache_t *)data;
  D_PRINT("[CACHE] path = %s\n", p->path);
}
