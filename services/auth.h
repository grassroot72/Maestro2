/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#ifndef _AUTH_H_
#define _AUTH_H_


typedef struct {
  char *id;
  char *pass;
} auth_t;


auth_t *auth_new();

void auth_delete(void *data);

int auth_compare(const void *curr,
                 const void *auth);

void auth_load_users(rbtree_t *authdb,
                     const char *passwdfile);

void auth_print(const void *data);


#endif
