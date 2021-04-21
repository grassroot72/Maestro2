/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include <string.h>
#include "xmalloc.h"
#include "http_header.h"

#define DEBUG
#include "debug.h"


httpheader_t *httpheader_new()
{
  httpheader_t *header = xmalloc(sizeof(httpheader_t));
  return header;
}

void httpheader_delete(void *data)
{
  httpheader_t *header = (httpheader_t *)data;
  if (header) {
    if (header->kvpair) xfree(header->kvpair);
  }
  xfree(data);
}

int httpheader_compare(const void *curr,
                       const void *header)
{
  char *s1 = ((httpheader_t *)curr)->kvpair;
  char *s2 = ((httpheader_t *)header)->kvpair;
  return strcmp(s1, s2);
}

void httpheader_print(const void *header)
{
  httpheader_t *p = (httpheader_t *)header;
  D_PRINT("[HEADER] %s: %s\n", p->kvpair, p->value);
}
