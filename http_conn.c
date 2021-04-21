/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <libpq-fe.h>
#include "xmalloc.h"
#include "util.h"
#include "io.h"
#include "sllist.h"
#include "rbtree.h"
#include "http_msg.h"
#include "http_parser.h"
#include "http_cfg.h"
#include "http_method.h"
#include "http_conn.h"

#define DEBUG
#include "debug.h"


#define SOCKET_KEEPALIVE_TIME 60000  /* 60 seconds */


httpconn_t *httpconn_new(const int sockfd,
                         const int epfd,
                         PGconn *pgconn,
                         rbtree_t *cache,
                         rbtree_t *timers,
                         rbtree_t *authdb,
                         httpcfg_t *cfg)
{
  httpconn_t *conn = xmalloc(sizeof(httpconn_t));
  conn->sockfd = sockfd;
  conn->epfd = epfd;
  conn->stamp = mstime();
  conn->pgconn = pgconn;
  conn->cache = cache;
  conn->timers = timers;
  conn->authdb = authdb;
  conn->cfg = cfg;
  return conn;
}

void httpconn_delete(void *conn)
{
  if (conn) {
    httpconn_t *c = (httpconn_t *)conn;
    D_PRINT("[CONN] server disconnected from socket %d\n", c->sockfd);
    /* explicitly remove the event from epoll */
    httpconn_epoll(conn, EPOLL_CTL_DEL);
    shutdown(c->sockfd, SHUT_RDWR);
    close(c->sockfd);
    xfree(c);
  }
}

int httpconn_epoll(httpconn_t *conn,
                   const int op)
{
  struct epoll_event event;
  event.data.ptr = (void *)conn;
  /* With the use of EPOLLONESHOT, it is guaranteed that a client
   * file descriptor is only used by one thread at a time */
  event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
  int rc = epoll_ctl(conn->epfd, op, conn->sockfd, &event);
  if (rc == -1) perror("[CONN] epoll_ctl");
  return rc;
}

int httpconn_compare(const void *curr,
                     const void *conn)
{
  httpconn_t *p1 = (httpconn_t *)curr;
  httpconn_t *p2 = (httpconn_t *)conn;
  return p1->sockfd - p2->sockfd;
}

void httpconn_task(void *arg)
{
  httpconn_t *conn = (httpconn_t *)arg;
  int rc;
  unsigned char *bytes = io_socket_read(conn->sockfd, &rc);

  /* rc = 0:  the client has closed the connection */
  if (rc == 0) {
    //D_PRINT("[CONN] client disconnected: %d\n", conn->sockfd);
    return;
  }

  if (rc == 1) {
    D_PRINT("[CONN] raw bytes:\n%s\n", bytes);
    if (bytes == NULL) return;

    httpmsg_t *req = http_parse_req(bytes);
    if (req == NULL) return;

    /* static GET */
    if (req->method == METHOD_GET || req->method == METHOD_HEAD) {
      http_get(conn->sockfd, conn->cache, req->path, conn->cfg, req);
    }

    /* POST */
    if (req->method == METHOD_POST) {
      http_post(conn->sockfd, conn->pgconn, conn->authdb, conn->cfg, req);
    }

    /* update timestamp for http-keepalive */
    conn->stamp = mstime();

    msg_delete(req, 1);
    xfree(bytes);
    /* put the event back */
    httpconn_epoll(conn, EPOLL_CTL_MOD);
    return;
  }
}

void httpconn_expire(void *arg)
{
  rbtree_t *timers = (rbtree_t *)arg;
  if (timers->size == 0) return;

  long curr_time = mstime();
  rbtrav_t *trav = rbtrav_new();
  void *data = rbtrav_first(trav, timers);
  httpconn_t *conn;

  do {
    if (data) {
      conn = (httpconn_t *)data;
      if (curr_time - conn->stamp > SOCKET_KEEPALIVE_TIME) {
        if (pthread_mutex_trylock(&timers->mutex) == 0) {
          /* remove the expired timer */
          rbtree_remove(timers, data);
          pthread_mutex_unlock(&timers->mutex);
        }
      }
    }
    data = rbtrav_next(trav);
  } while (data);

  rbtrav_delete(trav);
  D_PRINT("[RBTREE] size = %ld\n", timers->size);
}

void httpconn_print(const void *data)
{
  httpconn_t *p = (httpconn_t *)data;
  D_PRINT("[CONN] sockfd = %d, stamp = %ld\n", p->sockfd, p->stamp);
}
