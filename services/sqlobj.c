/* license: MIT
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include <string.h>
#include "xmalloc.h"
#include "memcpy_sse2.h"
#include "json.h"
#include "sqlobj.h"

#define DEBUG
#include "debug.h"


sqlobj_t *sqlobj_new()
{
  sqlobj_t *sqlo = xcalloc(1, sizeof(sqlobj_t));
  sqlo->viscols = 1;
  sqlo->nkeys = 0;
  return sqlo;
}

void sqlobj_destroy(sqlobj_t *sqlo)
{
  int i = 0;
  do {
    if (sqlo->keys[i]) xfree(sqlo->keys[i]);
    if (sqlo->values[i]) xfree(sqlo->values[i]);
    i++;
  } while (i < sqlo->nkeys);
  xfree(sqlo);
}

sqlobj_t *sql_parse_json(void *data)
{
  sqlobj_t *sqlo = sqlobj_new();

  struct json_object_element_s *e0 = (struct json_object_element_s *)data;
  struct json_string_s *e0_vs = json_value_as_string(e0->value);
  D_PRINT("value = %s\n", e0_vs->string);
  memcpy_fast(sqlo->statement, e0_vs->string, e0_vs->string_size);

  /* ex. "viscols":1 */
  struct json_object_element_s *e1 = e0->next;
  struct json_string_s *e1_name = e1->name;
  D_PRINT("[JSON] obj key = %s, ", e1_name->string);
  struct json_value_s *e1_value = e1->value;
  struct json_number_s *e1_vn = json_value_as_number(e1_value);
  D_PRINT("value = %s\n", e1_vn->number);
  sqlo->viscols = atoi(e1_vn->number);

  return sqlo;
}
