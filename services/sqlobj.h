/* license: MIT
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#ifndef _SQLOBJ_
#define _SQLOBJ_


#define MAX_SQL_KEYS 32


typedef struct {
  char statement[256];
  char *keys[MAX_SQL_KEYS];
  char *values[MAX_SQL_KEYS];
  int nkeys;
  int viscols;  /* field name visibility */
} sqlobj_t;


sqlobj_t *sqlobj_new();

void sqlobj_destroy(sqlobj_t *sqlo);

sqlobj_t *sql_parse_json(void *data);


#endif
