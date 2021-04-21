/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <libpq-fe.h>
#include <libdeflate.h>
#include "xmalloc.h"
#include "util.h"
#include "sllist.h"
#include "rbtree.h"
#include "json.h"
#include "jwt.h"
#include "auth.h"
#include "pg_conn.h"
#include "sqlobj.h"
#include "sqlops.h"
#include "http_msg.h"
#include "http_cfg.h"
#include "http_method.h"

#define DEBUG
#include "debug.h"


static void _add_common_headers(httpmsg_t *rep)
{
  msg_add_header(rep, "Server", SVR_VERSION);
  msg_add_header(rep, "Connection", "Keep-Alive");
  msg_add_header(rep, "Accept-Ranges", "bytes");

  char rep_date[30]; /* reply start date */
  time_t rep_time = time(NULL);
  gmt_date(rep_date, &rep_time);
  msg_add_header(rep, "Date", rep_date);
}

static void _wrong_user_pass(const int sockfd)
{
  httpmsg_t *rep = msg_new();
  msg_set_rep_line(rep, 1, 1, 200, "OK");
  _add_common_headers(rep);
  msg_add_header(rep, "Content-Type", "text/plain");
  msg_add_header(rep, "Content-Length", "20");

  msg_send_headers(sockfd, rep);
  msg_send_body(sockfd, (unsigned char *)"Wrong user/password!", 20);
  msg_delete(rep, 0);
}

static void _svc_dispatch(const int sockfd,
                          PGconn *pgconn,
                          rbtree_t *authdb,
                          const httpcfg_t *cfg,
                          const httpmsg_t *req)
{
  unsigned char *body = req->body;
  if (!body) return;
  D_PRINT("[REQ] json string:\n%s\n", (char *)body);

  struct json_value_s *root = json_parse(body, req->len_body);
  struct json_object_s *object = json_value_as_object(root);

  /* ex. {"SQL":"SELECT * FROM users", "viscols":1} */
  struct json_object_element_s *e0 = object->start;
  const char *key = ((struct json_string_s *)e0->name)->string;

  if (strcmp(key, "SQL") == 0) {
    sqlobj_t *sqlo = sql_parse_json(e0);
    xfree(root);

    char sqlres[2048];
    sql_fetch(sqlres, pgconn, sqlo);
    sqlobj_destroy(sqlo);

    httpmsg_t *rep = msg_new();
    msg_set_rep_line(rep, 1, 1, 200, "OK");
    _add_common_headers(rep);
    msg_add_header(rep, "Content-Type", "application/json");
    msg_add_header(rep, "Transfer-Encoding", "chunked");

    msg_send_headers(sockfd, rep);
    msg_send_body_chunk(sockfd, sqlres, strlen(sqlres));
    msg_send_body_end_chunk(sockfd);

    msg_delete(rep, 0);
  }

  if (strcmp(key, "Auth") == 0) {
    struct json_string_s *e0_vs = json_value_as_string(e0->value);
    char *id = (char *)e0_vs->string;
    char *pass = split_kv(id, '=');
    /* password in base64 format */
    D_PRINT("[JSON] id = %s, pass = %s\n", id, pass);

    /* process authentication */
    auth_t user;
    user.id = id;
    void *data = rbtree_search(authdb, &user);
    if (data) {
      auth_t *user = (auth_t *)data;
      D_PRINT("[AUTH] id = %s, pass = %s\n", user->id, user->pass);
      if (strcmp(user->pass, pass) != 0) {
        xfree(root);
        _wrong_user_pass(sockfd);
        return;
      }
      xfree(root);
    }
    else {
      xfree(root);
      _wrong_user_pass(sockfd);
      return;
    }

    char *jwt = jwt_gen_token(id, cfg->jwt_exp);
    /* token=xxx.yyy.zzz; HttpOnly
     * |-6--|           |---10---| */
    char cookie[256];
    char *ret;
    ret = strbld(cookie, "token=");
    ret = strbld(ret, jwt);
    ret = strbld(ret, ";HttpOnly");  /* against XSS */
    *ret++ = '\0';
    xfree(jwt);

    httpmsg_t *rep = msg_new();
    msg_set_rep_line(rep, 1, 1, 200, "OK");
    _add_common_headers(rep);
    msg_add_header(rep, "Content-Type", "text/plain");
    msg_add_header(rep, "Content-Length", "26");
    msg_add_header(rep, "Set-Cookie", cookie);
    D_PRINT("[JWT] %s\n", cookie);

    msg_send_headers(sockfd, rep);
    msg_send_body(sockfd, (unsigned char *)"You'v been authorized now!", 26);

    msg_delete(rep, 0);
  }

  xfree(root);
}

void http_post(const int sockfd,
               PGconn *pgconn,
               rbtree_t *authdb,
               const httpcfg_t *cfg,
               const httpmsg_t *req)
{
  char *ctype = msg_header_value(req, "Content-Type");
  if (strcmp(ctype, "application/json") == 0) {
    _svc_dispatch(sockfd, pgconn, authdb, cfg, req);
  }
}
