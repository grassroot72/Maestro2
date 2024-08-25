/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <libpq-fe.h>
#include <libdeflate.h>
#include "xmalloc.h"
#include "rbtree.h"
#include "util.h"
#include "auth.h"
#include "thpool.h"
#include "http_cfg.h"
#include "epsock.h"
#include "pg_conn.h"
#include "http_cache.h"
#include "http_conn.h"

#define DEBUG
#include "debug.h"


#define MAXEVENTS 2048
#define EPOLL_TIMEOUT 500 /* 0.5 second */
#define PORT 9000


static volatile int svc_running = 1;
static void _svc_stopper(int dummy)
{
  svc_running = 0;
}


int main(int argc, char **argv)
{
  /* set the http configure */
  httpcfg_t *cfg = httpcfg_new();

  /* create a postgresql db connection */
  /* PGconn *pgconn = pg_connect("dbname = demo", "identity"); */
  PGconn *pgconn = NULL;
  /* must called before calling libdeflate_alloc_compressor */
  libdeflate_set_memory_allocator(xmalloc, xfree);

  /* generate random nubmer seed */
  srandom(time(0));

  /* when a fd is closed by remote, writing to this fd will cause system
   * send SIGPIPE to this process, which exit the program */
  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  if (sigaction(SIGPIPE, &sa, NULL)) {
    D_PRINT("install sigal handler for SIGPIPE failed\n");
    return 0;
  }
  /* ctrl-c handler */
  sa.sa_handler = _svc_stopper;
  if (sigaction(SIGINT, &sa, NULL)) {
    D_PRINT("install sigal handler for SIGPIPE failed\n");
    return 0;
  }

  /* detect number of cpu cores and use it for thread pool */
  int np = get_nprocs();
  thpool_t *taskpool = thpool_new(np);

  /* list of files cached in the memory */
  rbtree_t *cache = rbtree_new(httpcache_compare,
                               httpcache_delete,
                               httpcache_print);
  /* timers */
  rbtree_t *timers = rbtree_new(httpconn_compare,
                                httpconn_delete,
                                httpconn_print);

  /* user database for authentication */
  rbtree_t *authdb = rbtree_new(auth_compare,
                                auth_delete,
                                auth_print);
  auth_load_users(authdb, "demo/users");

  /* loop time */
  long loop_time = mstime();

  /* listen on PORT */
  int srvfd = epsock_listen(PORT);

  /* create the epoll socket */
  int epfd = epoll_create1(0);
  if (epfd == -1) {
    perror("epoll_create1()");
    return -1;
  }

  /* mark the server socket for reading, and become edge-triggered */
  struct epoll_event event;
  httpconn_t *srvconn = httpconn_new(srvfd, epfd, NULL, NULL, NULL, NULL, NULL);
  event.data.ptr = (void *)srvconn;
  event.events = EPOLLIN | EPOLLET;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, srvfd, &event) == -1) {
    perror("epoll_ctl()");
    return -1;
  }

  struct epoll_event *events = xcalloc(MAXEVENTS, sizeof(struct epoll_event));
  do {
    int nevents = epoll_wait(epfd, events, MAXEVENTS, EPOLL_TIMEOUT);
    if (nevents == -1) {
      if (errno == EINTR) continue;
      perror("epoll_wait()");
    }

    if ((mstime() - loop_time) >= EPOLL_TIMEOUT) {
      /* expire the timers */
      thpool_add_task(taskpool, httpconn_expire, timers);
      /* expire the cache */
      thpool_add_task(taskpool, httpcache_expire, cache);
      loop_time = mstime();
    }

    /* loop through events */
    int i = 0;
    do {
      httpconn_t *conn = (httpconn_t *)events[i].data.ptr;
      /* error case */
      if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)) {
        if (errno == EAGAIN || errno == EINTR)
          nsleep(10);
        else {
          D_PRINT("[EPOLL] errno = %d, ", errno);
          perror("[ERR|HUP]");
          break;
        }
      }
      /* get input */
      if (events[i].events & EPOLLIN) {
        if (conn->sockfd == srvfd)
          epsock_connect(srvfd, epfd, pgconn, cache, timers, authdb, cfg);
        else {
          /* client socket; read client data and process it */
          thpool_add_task(taskpool, httpconn_task, conn);
        }
      }
      i++;
    } while (i < nevents);
  } while (svc_running);

  /* glibc doesn't free thread stacks when threads exit;
   * it caches them for reuse, and only prunes the cache when it gets huge.
   * Thus it always "leaks" some memory. So, don't worry about it. */
  thpool_delete(taskpool);

  rbtree_delete(timers);
  rbtree_print(cache);
  rbtree_delete(cache);
  rbtree_delete(authdb);

  shutdown(srvfd, SHUT_RDWR);
  close(srvfd);
  if (srvconn) xfree(srvconn);
  close(epfd);
  xfree(events);

  /* PQfinish(pgconn); */
  httpcfg_delete(cfg);

  D_PRINT("Exit gracefully...\n");
  return 0;
}
