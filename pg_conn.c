/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include "util.h"
#include "pg_conn.h"

//#define DEBUG
#include "debug.h"


void pg_exit_nicely(PGconn *conn)
{
  PQfinish(conn);
  exit(1);
}

PGconn *pg_connect(const char *conninfo,
                   const char *schema)
{
  /* Make a connection to the database */
  PGconn *conn = PQconnectdb(conninfo);

  /* Check to see that the backend connection was successfully made */
  if (PQstatus(conn) != CONNECTION_OK) {
    D_PRINT("[DB] Connection to database failed: %s\n", PQerrorMessage(conn));
    pg_exit_nicely(conn);
  }

  /* Set always-secure search path, so malicious users can't take control */
  char path[64];
  char *ret = strbld(path, "SET search_path=");
  ret = strbld(ret, schema);
  *ret++ = '\0';

  PGresult *res = PQexec(conn, path);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    D_PRINT("[DB] SET search_path failed: %s\n", PQerrorMessage(conn));
    PQclear(res);
    pg_exit_nicely(conn);
  }

  /* PQclear PGresult whenever it is no longer needed to avoid memory leaks */
  PQclear(res);

  return conn;
}
