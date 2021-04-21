/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#ifndef _HTTP_HEADER_H_
#define _HTTP_HEADER_H_


typedef struct {
  char *kvpair;
  char *value;
} httpheader_t;


httpheader_t *httpheader_new();

void httpheader_delete(void *data);

int httpheader_compare(const void *curr,
                       const void *header);

void httpheader_print(const void *header);


#endif
