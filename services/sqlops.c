/* license: MIT
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include <string.h>
#include <libpq-fe.h>
#include "io.h"
#include "util.h"
#include "pg_conn.h"
#include "sqlobj.h"
#include "sqlops.h"

#define DEBUG
#include "debug.h"


void _parse_result(char *res,
                   PGresult *pgres,
                   const int viscols)
{
  int i, j;

  int nFields = PQnfields(pgres);
  char *ret = strbld(res, "{");
  /* show attribute names? */
  if (viscols) {
    ret = strbld(ret, "\"h\":{\"hd\":[");
    for (i = 0; i < nFields; i++) {
      ret = strbld(ret, "\"");
      ret = strbld(ret, PQfname(pgres, i));
      if (i != nFields - 1)
        ret = strbld(ret, "\",");
      else
        ret = strbld(ret, "\"]},");
    }
  }

  /* next, show the row values */
  int nRows = PQntuples(pgres);
  for (i = 0; i < nRows; i++) {
    if (i == 0)
      ret = strbld(ret, "\"d\":{\"r");
    else
      ret = strbld(ret, "\"r");

    char tmp[64];
    sprintf(tmp, "%03d", i);
    ret = strbld(ret, tmp);
    ret = strbld(ret, "\":[");

    for (j = 0; j < nFields; j++) {
      *ret++ = '"';
      ret = strbld(ret, PQgetvalue(pgres, i, j));
      if (j != nFields - 1)
        ret = strbld(ret, "\",");
      else {
        if (i != nRows - 1)
          ret = strbld(ret, "\"],");
        else
          ret = strbld(ret, "\"]}");
      }
    }
  }
  *ret++ = '}';
  *ret++ = '\0';
  PQclear(pgres);
}

void sql_select(char *res,
                PGconn *pgconn,
                const sqlobj_t *sqlo)
{
  const char *sql = sqlo->statement;

  /* Start a transaction block */
  PGresult *pgres = PQexec(pgconn, "BEGIN");
  if (PQresultStatus(pgres) != PGRES_COMMAND_OK) {
    D_PRINT("BEGIN command failed: %s\n", PQerrorMessage(pgconn));
    PQclear(pgres);
    pg_exit_nicely(pgconn);
  }
  PQclear(pgres);

  /* prepare statement name */
  const char *stmt = "prep_select";
  pgres = PQprepare(pgconn,
                    stmt,
                    sql,
                    0,
                    NULL);
  if (PQresultStatus(pgres) != PGRES_COMMAND_OK) {
    D_PRINT("PREPARE failed: %s\n", PQerrorMessage(pgconn));
    PQclear(pgres);
    pg_exit_nicely(pgconn);
  }
  PQclear(pgres);

  /* execute the prepared statement */
  pgres = PQexecPrepared(pgconn,
                         stmt,
                         0,
                         NULL,
                         NULL,
                         NULL,
                         0);  /* text result */
  if (PQresultStatus(pgres) != PGRES_TUPLES_OK) {
    D_PRINT("SELECT failed: %s\n", PQerrorMessage(pgconn));
    PQclear(pgres);
    pg_exit_nicely(pgconn);
  }

  /* parse the result set */
  _parse_result(res, pgres, sqlo->viscols);
  D_PRINT("[SQL] pg result: %s\n", res);

  /* Deallocate all prepared statements */
  pgres = PQexec(pgconn, "DEALLOCATE ALL");
  PQclear(pgres);

  /* end the transaction */
  pgres = PQexec(pgconn, "END");
  PQclear(pgres);
}

void sql_fetch(char *res,
               PGconn *pgconn,
               const sqlobj_t *sqlo)
{
  char sql[256];
  char *ret = strbld(sql, "DECLARE portal CURSOR FOR ");
  ret = strbld(ret, sqlo->statement);
  *ret++ = '\0';

  /* Start a transaction block */
  PGresult *pgres = PQexec(pgconn, "BEGIN");
  if (PQresultStatus(pgres) != PGRES_COMMAND_OK) {
    D_PRINT("BEGIN command failed: %s\n", PQerrorMessage(pgconn));
    PQclear(pgres);
    pg_exit_nicely(pgconn);
  }
  PQclear(pgres);

  /* prepare statement name */
  const char *stmt = "prep_cursor";
  pgres = PQprepare(pgconn,
                    stmt,
                    sql,
                    0,
                    NULL);
  if (PQresultStatus(pgres) != PGRES_COMMAND_OK) {
    D_PRINT("PREPARE failed: %s\n", PQerrorMessage(pgconn));
    PQclear(pgres);
    pg_exit_nicely(pgconn);
  }
  PQclear(pgres);

  /* execute the prepared statement */
  pgres = PQexecPrepared(pgconn,
                         stmt,
                         0,
                         NULL,
                         NULL,
                         NULL,
                         0);  /* text result */
  if (PQresultStatus(pgres) != PGRES_COMMAND_OK) {
    D_PRINT("OPEN CURSOR failed: %s\n", PQerrorMessage(pgconn));
    PQclear(pgres);
    pg_exit_nicely(pgconn);
  }
  PQclear(pgres);

  pgres = PQexec(pgconn, "FETCH ALL in portal");
  if (PQresultStatus(pgres) != PGRES_TUPLES_OK) {
    D_PRINT("FETCH ALL failed: %s\n", PQerrorMessage(pgconn));
    PQclear(pgres);
    pg_exit_nicely(pgconn);
  }

  /* parse the result set */
  _parse_result(res, pgres, sqlo->viscols);
  D_PRINT("[SQL] pg result: %s\n", res);

  /* Deallocate all prepared statements */
  pgres = PQexec(pgconn, "DEALLOCATE ALL");
  PQclear(pgres);

  /* close the portal ... we don't bother to check for errors ... */
  pgres = PQexec(pgconn, "CLOSE portal");
  PQclear(pgres);

  /* end the transaction */
  pgres = PQexec(pgconn, "END");
  PQclear(pgres);
}
