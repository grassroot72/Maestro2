/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include "xmalloc.h"
#include "http_cfg.h"


#define CACHE_MAX_AGE 300000 /* ms */


httpcfg_t *httpcfg_new()
{
  httpcfg_t *c = xmalloc(sizeof(httpcfg_t));
  c->max_age = CACHE_MAX_AGE;
  c->jwt_exp = 86400;  /* 86400 = 24 hrs */
  return c;
}

void httpcfg_delete(httpcfg_t *c)
{
  xfree(c);
}
