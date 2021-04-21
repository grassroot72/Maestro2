/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#ifndef _HTTP_METHOD_H_
#define _HTTP_METHOD_H_


/* GET */
void http_get(const int sockfd,
              rbtree_t *cache,
              char *path,
              const httpcfg_t *cfg,
              const httpmsg_t *req);

/* POST */
void http_post(const int sockfd,
               PGconn *pgconn,
               rbtree_t *authdb,
               const httpcfg_t *cfg,
               const httpmsg_t *req);


#endif
