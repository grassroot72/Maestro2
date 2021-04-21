/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include <string.h>
#include "xmalloc.h"
#include "memcpy_sse2.h"
#include "util.h"
#include "rbtree.h"
#include "auth.h"

#define DEBUG
#include "debug.h"


auth_t *auth_new()
{
  auth_t *data = xmalloc(sizeof(auth_t));
  return data;
}

void auth_delete(void *data)
{
  if (data) {
    auth_t *user = (auth_t *)data;
    xfree(user->id);
    xfree(user);
    D_PRINT("[AUTH] user deleted...\n");
  }
}

int auth_compare(const void *curr,
                 const void *user)
{
  char *s1 = ((auth_t *)curr)->id;
  char *s2 = ((auth_t *)user)->id;
  return strcmp(s1, s2);
}

void auth_load_users(rbtree_t *authdb,
                     const char *passwdfile)
{
  FILE *fp;
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;

  fp = fopen(passwdfile, "r");
  if (fp == NULL) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  while ((nread = getline(&line, &len, fp)) != -1) {
    /* line[nread-1] is LF, so we set it to '\0' as the string end */
    int len_kv = nread - 1;
    line[len_kv] = '\0';
    char *id = xmalloc(len_kv);
    memcpy_fast(id, line, len_kv);
    auth_t *user = auth_new();
    user->id = id;
    user->pass = split_kv(id, '=');
    D_PRINT("[AUTH] id = %s, pass = %s\n", user->id, user->pass);
    rbtree_insert(authdb, user);
  }

  free(line);
  fclose(fp);
}

void auth_print(const void *data)
{
  auth_t *p = (auth_t *)data;
  D_PRINT("[AUTH] user = %s, pass = %s\n", p->id, p->pass);
}
