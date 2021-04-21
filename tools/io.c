/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include "xmalloc.h"
#include "memcpy_sse2.h"
#include "util.h"
#include "io.h"

#define DEBUG
#include "debug.h"


#define CHUNK_SIZE 2048
#define BUF_MAX_SIZE 8192


unsigned char *io_socket_read(const int sockfd,
                              int *rc)
{
  int n;
  unsigned char buf[CHUNK_SIZE];
  unsigned char workbuf[BUF_MAX_SIZE];
  unsigned char *last;
  int last_sz;

  unsigned char *bytes = NULL;

  workbuf[0] = '\0';
  last = workbuf;
  last_sz = 0;

  /* use loop to read as much as possible in a task */
  for (;;) {
    n = recv(sockfd, buf, CHUNK_SIZE, 0);
    /* the client close the socket: EOF reached */
    if (n == 0) {
      *rc = 0;
      return NULL;
    }

    if (n == -1) {
      /* normally errno = EAGAIN, this is expected behaviour */
      *rc = 1;
      return NULL;
    }

    memcpy_fast(last, buf, n);
    last += n;
    last_sz += n;

    if (n < CHUNK_SIZE) {
      *last = '\0';
      last_sz++;
      bytes = xmalloc(last_sz);
      memcpy_fast(bytes, workbuf, last_sz);
      *rc = 1;
      return bytes;
    }

    nsleep(10);
  }
}

void io_socket_write(const int sockfd,
                     const unsigned char *bytes,
                     const size_t len)
{
  int n;
  const unsigned char *last;
  size_t done_sz;
  size_t left_sz;

  last = bytes;
  done_sz = 0;

  /* use loop to write as much as possible in a task */
  do {
    left_sz = len - done_sz;
    if (left_sz <= 0) return;

    n = send(sockfd, last, left_sz, 0);
    if (n == -1) {
      /* perror("write()") */
      if (errno == EPIPE) return;
      nsleep(10);
      continue;
    }

    last += n;
    done_sz += n;
  } while (1);
}

unsigned char *io_fread(const char *fname,
                        const size_t len)
{
  FILE *f;
  unsigned char *buf;

  buf = xmalloc(len);
  f = fopen(fname, "r");
  fread(buf, 1, len, f);
  fclose(f);

  return buf;
}

unsigned char *io_fread_pipe(FILE *f,
                             const size_t len)
{
  unsigned char *buf;

  buf = xmalloc(len);
  fread(buf, 1, len, f);
  fclose(f);

  return buf;
}

char *io_fgetc(FILE *f,
               int *len)
{
  int capacity = 10;
  int index = 0;
  char *buf = xmalloc(capacity);

  char *newbuf;
  int c;
  while ((c = fgetc(f)) != EOF) {
    assert(index < capacity);
    buf[index++] = c;

    if (index == capacity) {
      newbuf = xmalloc(capacity << 1);
      memcpy_fast(newbuf, buf, capacity);
      xfree(buf);
      buf = newbuf;
      capacity *= 2;
    }
  }

  buf[index] = '\0';
  *len = index;
  fclose(f);

  return buf;
}
