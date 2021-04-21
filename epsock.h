/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#ifndef _EPSOCK_H_
#define _EPSOCK_H_


void epsock_connect(const int srvfd,
                    const int epfd,
                    PGconn *pgconn,
                    rbtree_t *cache,
                    rbtree_t *timers,
                    rbtree_t *authdb,
                    httpcfg_t *cfg);

int epsock_listen(const uint16_t port);


#endif
