/* license: MIT
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#ifndef _SQLOPS_
#define _SQLOPS_


void sql_select(char *res,
                PGconn *pgconn,
                const sqlobj_t *sqlo);

void sql_fetch(char *res,
               PGconn *pgconn,
               const sqlobj_t *sqlo);


#endif
