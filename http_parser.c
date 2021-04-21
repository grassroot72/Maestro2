/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xmalloc.h"
#include "sllist.h"
#include "http_msg.h"
#include "http_parser.h"

#define DEBUG
#include "debug.h"


httpmsg_t *http_parse_req(const unsigned char *buf)
{
  unsigned char *startline;
  httpmsg_t *req = msg_new();
  int n = msg_parse(req->headers, &startline, &req->body, &req->len_body, buf);
  D_PRINT("[PARSER] number of headers (include startline) = %d\n", n);
  if (n < 3) {
    D_PRINT("[PARSER] not a valid message\n");
    msg_delete(req, 1);
    return NULL;
  }

  /* request line ... */
  D_PRINT("[PARSER] startline: %s\n", startline);
  char *rest = (char *)startline;
  char *method = strtok_r(rest, " ", &rest);
  char *path = strtok_r(NULL, " ", &rest);
  char *version = strtok_r(NULL, " ", &rest);
  int major = version[5] - '0';
  int minor = version[7] - '0';

  if (strcmp(path, "/") == 0)
    msg_set_req_line(req, method, "/demo/index.html", major, minor);
  else
    msg_set_req_line(req, method, path, major, minor);

  xfree(startline);
  return req;
}

httpmsg_t *http_parse_rep(const unsigned char *buf)
{
  unsigned char *startline;
  httpmsg_t *rep = msg_new();
  int n = msg_parse(rep->headers, &startline, &rep->body, &rep->len_body, buf);
  D_PRINT("[PARSER] number of headers (include startline) = %d\n", n);
  if (n < 3) {
    D_PRINT("[PARSER] not a valid message\n");
    msg_delete(rep, 1);
    return NULL;
  }

  /* status line ... */
  D_PRINT("[PARSER] startline: %s\n", startline);
  char *rest = (char *)startline;
  char *version = strtok_r(rest, " ", &rest);
  int major = version[5] - '0';
  int minor = version[7] - '0';
  int code = atoi(strtok_r(NULL, " ", &rest));
  char *status = strtok_r(NULL, " ", &rest);

  msg_set_rep_line(rep, major, minor, code, status);

  xfree(startline);
  return rep;
}
