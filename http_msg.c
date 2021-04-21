/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "xmalloc.h"
#include "sllist.h"
#include "memcpy_sse2.h"
#include "util.h"
#include "io.h"
#include "http_header.h"
#include "http_msg.h"

#define DEBUG
#include "debug.h"


#define LF '\n'
#define CR '\r'


httpmsg_t *msg_new()
{
  httpmsg_t *msg = xmalloc(sizeof(httpmsg_t));
  msg->headers = sll_new(httpheader_compare,
                         httpheader_delete,
                         httpheader_print);
  msg->len_startline = 0;
  msg->len_headers = 0;

  msg->method = METHOD_GET;
  msg->path = NULL;
  msg->status = NULL;

  msg->body = NULL;
  msg->body_zipped = NULL;
  return msg;
}

void msg_delete(httpmsg_t *msg,
                const int delbody)
{
  if (!msg) return;
  if (msg->path) xfree(msg->path);
  if (msg->status) xfree(msg->status);

  sll_delete(msg->headers);

  if (delbody) {
    if (msg->body) xfree(msg->body);
    if (msg->body_zipped) xfree(msg->body_zipped);
  }
  xfree(msg);
}

char *msg_header_value(const httpmsg_t *msg,
                       char *key)
{
  httpheader_t _h;
  _h.kvpair = key;
  httpheader_t *h = sll_search(msg->headers, &_h);
  if (h == NULL) return NULL;
  return h->value;
}

int msg_parse(sllist_t *headers,
              unsigned char **startline,
              unsigned char **body,
              size_t *len_body,
              const unsigned char *buf)
{
  const unsigned char *p = buf;
  if (*p == CR || *p == LF) {
    return 0;  /* empty message */
  }

  const unsigned char *h = p;
  int i = 0;
  int len = 0;
  int size;
  do {
    if (*p == LF) {

      /*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxx\r\n
       *  ^                             ^
       *  h                             p
       *
       *  xxxxxxxxxxxxxxxxxxxxx\r\n
       *  ^
       *  h=p+1 */
      size = p - h;
      if (size == 0) return 0;
      len = size - 1;
      if (len == 0) {  /* end of headers */
        if (i < 3) return 0;
        h = p + 1;
        break;
      }
      else {
        char *line = xmalloc(size);
        memcpy_fast(line, h, len);
        line[len] = '\0';
        if (i == 0) {
          *startline = (unsigned char *)line;
        }
        else {
          httpheader_t *header = httpheader_new();
          header->kvpair = line;
          header->value = split_kv(line, ':');
          D_PRINT("[MSG] k = %s ", header->kvpair);
          D_PRINT("value = %s\n", header->value);
          sll_lpush(headers, header);
        }
      }
      h = p + 1;
      i++;
    }
    p++;
  } while (*p);

  /* body */
  do { p++; } while (*p);
  *len_body = p - h;
  if (*len_body) {
    *body = xmalloc(*len_body);
    memcpy_fast(*body, h, *len_body);
  }
  return i;
}

void msg_add_header(httpmsg_t *msg,
                    const char *key,
                    const char *value)
{
  int len_k = strlen(key);
  int len_v = strlen(value);
  httpheader_t *header = httpheader_new();
  int len = len_k + len_v + 2;
  header->kvpair = xmalloc(len + 1);
  memcpy_fast(header->kvpair, key, len_k);
  char *p = header->kvpair + len_k;
  *p++ = ':';
  *p++ = ' ';
  memcpy_fast(p, value, len_v);
  header->kvpair[len] = '\0';
  sll_lpush(msg->headers, header);

  /* Host: www.xxxxx.com\r\n
   *     ^^              ^ ^
   *     11              1 1  (1+1+1+1 = 4) */
  msg->len_headers += len + 2;
}

/* ex. response */
int msg_headers_len(const httpmsg_t *msg)
{
  /* HTTP/1.1 200 OK\r\n
   * Host: www.xxxxx.com\r\n
   * Server: Mass/1.0\r\n
   * \r\n\0
   *  ^ ^ ^
   *  1 1 1  (1+1 = 2)
   * body .... */
  return msg->len_startline + msg->len_headers + 2;
}

/*------------------------- request header -----------------------------------*/
void msg_set_req_line(httpmsg_t *msg,
                      const char *method,
                      const char *path,
                      const int major,
                      const int minor)
{
  int len, total = 0;

  if (strcmp(method, "GET") == 0) {
    msg->method = METHOD_GET;
    len = 3;
  }
  if (strcmp(method, "POST") == 0) {
    msg->method = METHOD_POST;
    len = 4;
  }
  if (strcmp(method, "HEAD") == 0) {
    msg->method = METHOD_HEAD;
    len = 4;
  }
  total += len;

  len = strlen(path);
  msg->path = xmalloc(len + 1);
  memcpy_fast(msg->path, path, len);
  msg->path[len] = '\0';
  total += len;

  msg->ver_major = major;
  msg->ver_minor = minor;

  /* GET xxxxx HTTP/1.1\r\n
   *    ^     ^         ^ ^
   *    1     1|--8---| 1 1  (1+1+8+1+1 = 12) */
  total += 12;
  msg->len_startline = total;
}

void msg_req_headers(char *headerbytes,
                     const httpmsg_t *req)
{
  char *ret = headerbytes;

  /* start line */
  if (req->method == METHOD_GET) ret = strbld(ret, "GET ");
  if (req->method == METHOD_POST) ret = strbld(ret, "POST ");
  if (req->method == METHOD_HEAD) ret = strbld(ret, "HEAD ");

  ret = strbld(ret, req->path);
  ret = strbld(ret, " HTTP/");
  *ret++ = req->ver_major + '0';
  *ret++ = '.';
  *ret++ = req->ver_minor + '0';
  *ret++ = CR;
  *ret++ = LF;

  /* headers */
  slnode_t *n = req->headers->head;
  while (n != NULL) {
    httpheader_t *h = (httpheader_t *)n->data;
    ret = strbld(ret, h->kvpair);
    *ret++ = CR;
    *ret++ = LF;
    n = n->next;
  }
  /* ending CRLF */
  *ret++ = CR;
  *ret++ = LF;
}


/*------------------------- response header ----------------------------------*/
void msg_set_rep_line(httpmsg_t *msg,
                      const int major,
                      const int minor,
                      const int code,
                      const char *status)
{
  msg->ver_major = major;
  msg->ver_minor = minor;
  msg->code = code;

  int len = strlen(status);
  msg->status = xmalloc(len + 1);
  memcpy_fast(msg->status, status, len);
  msg->status[len] = '\0';;

  /* HTTP/1.1 200 OK\r\n
   *         ^   ^   ^ ^
   * |--8---|1|3|1   1 1  (8+1+3+1+1+1 = 15) */
  msg->len_startline = len + 15;
}

void msg_rep_headers(char *headerbytes,
                     const httpmsg_t *rep)
{
  unsigned char code[16];
  char *ret;

  /* start line */
  ret = strbld(headerbytes, "HTTP/");
  *ret++ = rep->ver_major + '0';
  *ret++ = '.';
  *ret++ = rep->ver_minor + '0';
  *ret++ = ' ';
  itos(code, rep->code, 10, ' ');
  ret = strbld(ret, (char *)code);
  *ret++ = ' ';
  ret = strbld(ret, rep->status);
  *ret++ = CR;
  *ret++ = LF;

  /* headers */
  slnode_t *n = rep->headers->head;
  while (n != NULL) {
    httpheader_t *h = (httpheader_t *)n->data;
    ret = strbld(ret, h->kvpair);
    *ret++ = CR;
    *ret++ = LF;
    n = n->next;
  }
  /* ending CRLF */
  *ret++ = CR;
  *ret++ = LF;
}


/*---------------------------- message body ----------------------------------*/
void msg_set_body_start(httpmsg_t *msg,
                        unsigned char *s)
{
  msg->body_s = s;
}

void msg_add_body(httpmsg_t *msg,
                  unsigned char *body,
                  const size_t len)
{
  msg->body = body;
  msg->len_body = len;
}

void msg_add_zipped_body(httpmsg_t *msg,
                         unsigned char *body_zipped,
                         const size_t len)
{
  msg->body_zipped = body_zipped;
  msg->len_body = len;
}


/*---------------------- message send ----------------------------------------*/
void msg_send_headers(const int sockfd,
                      const httpmsg_t *msg)
{
  int len_headers = msg_headers_len(msg);
  char *headerbytes = xmalloc(len_headers);
  msg_rep_headers(headerbytes, msg);

  /* send header */
  D_PRINT("[MSG] Sending msg headers... %d\n", sockfd);
  io_socket_write(sockfd, (unsigned char *)headerbytes, len_headers);
  xfree(headerbytes);
}

void msg_send_body(const int sockfd,
                   const unsigned char *data,
                   const int len_data)
{
  io_socket_write(sockfd, data, len_data);
}

void msg_send_body_chunk(const int sockfd,
                         const char *chunk,
                         const int len_chunk)
{
  unsigned char hex_len[16];
  int len = itos(hex_len, len_chunk, 16, ' ');
  /* chunked length in Hex */
  D_PRINT("[MSG] Sending the chunk length on socket %d\n", sockfd);
  io_socket_write(sockfd, hex_len, len);
  io_socket_write(sockfd, (unsigned char *)"\r\n", 2);
  /* chunk */
  D_PRINT("[MSG] Sending the chunk on socket %d\n", sockfd);
  io_socket_write(sockfd, (unsigned char *)chunk, len_chunk);
  io_socket_write(sockfd, (unsigned char *)"\r\n", 2);
}

void msg_send_body_end_chunk(const int sockfd)
{
  /* terminating the chuncked transfer */
  D_PRINT("[MSG] Sending the terminating chunk on socket %d\n", sockfd);
  io_socket_write(sockfd, (unsigned char *)"0\r\n\r\n", 5);
}
