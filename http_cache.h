/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#ifndef _HTTP_CACHE_H_
#define _HTTP_CACHE_H_


typedef struct {
  char *path;
  char *etag;
  char *last_modified;
  long stamp;

  unsigned char *body;
  unsigned char *body_zipped;
  size_t len_body;
  size_t len_zipped;
} httpcache_t;


httpcache_t *httpcache_new();

void httpcache_set(httpcache_t *data,
                   char *path,
                   char *etag,
                   char *modified,
                   unsigned char *body,
                   const size_t len_body,
                   unsigned char *body_zipped,
                   const size_t len_zipped);

void httpcache_clear(void *data);

void httpcache_delete(void *data);

int httpcache_compare(const void *curr,
                      const void *cache);

void httpcache_expire(void *arg);

void httpcache_print(const void *data);


#endif
