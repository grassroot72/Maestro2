/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#ifndef _HTTPCONN_H_
#define _HTTPCONN_H_


typedef struct {
  int sockfd;
  int epfd;
  long stamp;

  PGconn *pgconn;
  rbtree_t *cache;
  rbtree_t *timers;
  rbtree_t *authdb;
  httpcfg_t *cfg;
} httpconn_t;


httpconn_t *httpconn_new(const int sockfd,
                         const int epfd,
                         PGconn *pgconn,
                         rbtree_t *cache,
                         rbtree_t *timers,
                         rbtree_t *authdb,
                         httpcfg_t *cfg);

void httpconn_delete(void *conn);

int httpconn_epoll(httpconn_t *conn,
                   const int op);

int httpconn_compare(const void *curr,
                     const void *conn);

void httpconn_task(void *arg);

void httpconn_expire(void *arg);

void httpconn_print(const void *data);


#endif
