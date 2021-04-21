/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#ifndef _HTTP_CFG_H_
#define _HTTP_CFG_H_


typedef struct {
  long max_age;
  long jwt_exp;
} httpcfg_t;


httpcfg_t *httpcfg_new();

void httpcfg_delete(httpcfg_t *c);


#endif
