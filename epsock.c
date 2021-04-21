/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <libpq-fe.h>
#include "rbtree.h"
#include "pg_conn.h"
#include "http_cfg.h"
#include "http_conn.h"
#include "epsock.h"

#define DEBUG
#include "debug.h"


static void _set_nonblocking(const int fd)
{
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    perror("fcntl()");
    return;
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    perror("fcntl()");
}

void epsock_connect(const int srvfd,
                    const int epfd,
                    PGconn *pgconn,
                    rbtree_t *cache,
                    rbtree_t *timers,
                    rbtree_t *authdb,
                    httpcfg_t *cfg)
{
  struct sockaddr cliaddr;
  socklen_t len_cliaddr = sizeof(struct sockaddr);

   /* server socket; accept connections */
  for (;;) {
    int clifd = accept(srvfd, &cliaddr, &len_cliaddr);

    if (clifd == -1) {
      if (errno == EINTR) continue;
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        /* we processed all of the connections */
        break;
      }
      perror("accept()");
      close(clifd);
      break;
    }

    char *cli_ip = inet_ntoa(((struct sockaddr_in *)&cliaddr)->sin_addr);
    D_PRINT("[CONN] client %s connected on socket %d\n", cli_ip, clifd);

    _set_nonblocking(clifd);

    httpconn_t *cliconn = httpconn_new(clifd, epfd,
                                       pgconn, cache, timers, authdb,
                                       cfg);
    /* install the new timer */
    pthread_mutex_lock(&timers->mutex);
    rbtree_insert(timers, cliconn);
    pthread_mutex_unlock(&timers->mutex);

    if (httpconn_epoll(cliconn, EPOLL_CTL_ADD) == -1) return;
  }
}

static int _srv_socket()
{
  int srvfd = socket(AF_INET, SOCK_STREAM, 0);
  if (srvfd == -1) {
    perror("socket()");
    return -1;
  }

  int opt = 1;
  if (setsockopt(srvfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
    perror("setsockopt()");
    return -1;
  }
  return srvfd;
}

static int _bind(const uint16_t port)
{
  struct sockaddr_in srvaddr;
  //memset(&srvaddr, 0, sizeof(struct sockaddr_in));
  srvaddr.sin_family = AF_INET;
  srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  srvaddr.sin_port = htons(port);

  int srvfd = _srv_socket();
  if (bind(srvfd, (struct sockaddr*)&srvaddr, sizeof(struct sockaddr_in)) < 0) {
    perror("bind()");
    return -1;
  }
  return srvfd;
}

int epsock_listen(const uint16_t port)
{
  int srvfd = _bind(port);
  if (srvfd == -1) exit(1);
  _set_nonblocking(srvfd);
  if (listen(srvfd, SOMAXCONN) < 0) {
    perror("listen()");
    exit(1);
  }
  printf("listening on port [%d]\n", port);
  return srvfd;
}
