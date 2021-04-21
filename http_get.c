/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <libpq-fe.h>
#include <libdeflate.h>
#include "xmalloc.h"
#include "io.h"
#include "util.h"
#include "sllist.h"
#include "rbtree.h"
#include "thpool.h"
#include "jwt.h"
#include "base64.h"
#include "http_msg.h"
#include "http_cache.h"
#include "http_cfg.h"
#include "http_method.h"

//#define DEBUG
#include "debug.h"


#define MAX_PATH 256
#define MAX_CWD 64

static const char *content_type[] = {
  "image/png",
  "image/jpeg",
  "image/gif",
  "image/bmp",
  "image/x-icon",
  "image/webp",
  "image/svg+xml",
  "application/pdf",
  "application/gzip",
  "text/css",
  "text/html; charset=utf-8",
  "text/plain; charset=utf-8",
  "application/javascript",
  "application/json",
  "application/octet-stream"
};

enum mimetype {
  PNG,
  JPG,
  GIF,
  BMP,
  ICO,
  WEBP,
  SVG,
  PDF,
  GZ,
  CSS,
  HTML,
  TXT,
  JS,
  JSON,
  BIN
};

#define MIME_BIN 0   /* don't zip this type of data */
#define MIME_TXT 1


static int _find_content_type(int *ctype,
                              const char *ext)
{
  /* MIME types - images */
  if (strcmp(ext, "png") == 0) {
    *ctype = PNG;
    return MIME_BIN;
  }
  if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0 ||
      strcmp(ext, "jpe") == 0 || strcmp(ext, "jfif") == 0 ||
      strcmp(ext, "pjp") == 0) {
    *ctype = JPG;
    return MIME_BIN;
  }
  if (strcmp(ext, "gif") == 0) {
    *ctype = GIF;
    return MIME_BIN;
  }
  if (strcmp(ext, "bmp") == 0) {
    *ctype = BMP;
    return MIME_BIN;
  }
  if (strcmp(ext, "ico") == 0 || strcmp(ext, "cur") == 0) {
    *ctype = ICO;
    return MIME_BIN;
  }
  if (strcmp(ext, "webp") == 0) {
    *ctype = WEBP;
    return MIME_BIN;
  }
  if (strcmp(ext, "svg") == 0) {
    *ctype = SVG;
    return MIME_TXT;
  }

  /* MIME type - pdf */
  if (strcmp(ext, "pdf") == 0) {
    *ctype = PDF;
    return MIME_BIN;
  }

  /* MIME type - gz */
  if (strcmp(ext, "gz") == 0) {
    *ctype = GZ;
    return MIME_BIN;
  }

  /* MIME type - css */
  if (strcmp(ext, "css") == 0) {
    *ctype = CSS;
    return MIME_TXT;
  }

  /* MIME types - javascripts */
  if (strcmp(ext, "js") == 0 || strcmp(ext, "mjs") == 0) {
    *ctype = JS;
    return MIME_TXT;
  }

  /* MIME type - html */
  if (strcmp(ext, "html") == 0 || strcmp(ext, "htm") == 0) {
    *ctype = HTML;
    return MIME_TXT;
  }

  /* MIME type - plain text */
  if (strcmp(ext, "txt") == 0) {
    *ctype = TXT;
    return MIME_TXT;
  }

  *ctype = BIN;
  return MIME_BIN;
}

static httpmsg_t *_401_unauthorized(const char *path,
                                    const char *msg)
{
  httpmsg_t *rep = msg_new();
  D_PRINT("[SYS] <%s> User not authenticated!\n", path);
  char len_str[16];
  /* len of <html><body> + </body></html> is 26 */
  int len_body = 26 + strlen(msg);
  char *body = xmalloc(len_body + 1);
  char *ret = strbld(body, "<html><body>");
  ret = strbld(ret, msg);
  ret = strbld(ret, "</body></html>");
  *ret++ = '\0';
  msg_set_rep_line(rep, 1, 1, 401, "Unauthorized");
  msg_add_body(rep, (unsigned char *)body, len_body);
  msg_set_body_start(rep, (unsigned char *)body);
  itos((unsigned char *)len_str, len_body, 10, ' ');
  D_PRINT("[401] body: \n%s\n", body);
  msg_add_header(rep, "Content-Length", len_str);
  return rep;
}

static httpmsg_t *_403_forbidden(const char *path)
{
  httpmsg_t *rep = msg_new();
  D_PRINT("[SYS] <%s> Directory access forbbiden\n", path);
  char *body = xstrdup("<html><body>403 Forbidden</body></html>");
  msg_set_rep_line(rep, 1, 1, 403, "Forbidden");
  msg_add_body(rep, (unsigned char *)body, 39);
  msg_set_body_start(rep, (unsigned char *)body);
  msg_add_header(rep, "Content-Length", "39");
  return rep;
}

static httpmsg_t *_404_not_found(const char *path)
{
  httpmsg_t *rep = msg_new();
  D_PRINT("[SYS] <%s> No such file or directory\n", path);
  char *body = xstrdup("<html><body>404 Not Found</body></html>");
  msg_set_rep_line(rep, 1, 1, 404, "Not Found");
  msg_add_body(rep, (unsigned char *)body, 39);
  msg_set_body_start(rep, (unsigned char *)body);
  msg_add_header(rep, "Content-Length", "39");
  return rep;
}

static httpmsg_t *_302_found(const char *path)
{
  httpmsg_t *rep = msg_new();
  D_PRINT("[GET_REP] redirect to <%s>\n", path);
  char *body = xstrdup("<html><body>302 Found</body></html>");
  msg_set_rep_line(rep, 1, 1, 302, "Found");
  msg_add_header(rep, "Location", path);
  msg_add_body(rep, (unsigned char *)body, 35);
  msg_set_body_start(rep, (unsigned char *)body);
  msg_add_header(rep, "Content-Length", "35");
  return rep;
}

static httpmsg_t *_304_not_modified()
{
  httpmsg_t *rep = msg_new();
  char *body = xstrdup("<html><body>302 Not Modified</body></html>");
  msg_set_rep_line(rep, 1, 1, 304, "Not Modified");
  msg_add_body(rep, (unsigned char *)body, 42);
  msg_set_body_start(rep, (unsigned char *)body);
  msg_add_header(rep, "Content-Length", "42");
  return rep;
}

static size_t _process_range(httpmsg_t *rep,
                             char *range_str,
                             size_t *len_range,
                             const size_t len_body)
{
  size_t range_si;  /* range start */
  size_t range_ei;  /* range end */
  char range[64];

  char *range_s = split_kv(range_str, '=');
  char *range_e = split_kv(range_s, '-');

  range_si = atoi(range_s);
  /* req: bytes=xxxx-xxxx */
  if (*range_e) {
    range_ei = atol(range_e);
    *len_range = range_ei - range_si + 1;
    sprintf(range, "bytes %lu-%lu/%lu", range_si, range_ei, len_body);
  }
  /* req: bytes=xxxx- */
  else {
    *len_range = len_body - range_si;
    //sprintf(range, "bytes %lu-%lu/%lu", range_si, len_body - 1, len_body);
    sprintf(range, "bytes %lu-/%lu", range_si, len_body);
  }

  msg_add_header(rep, "Content-Range", range);
  //D_PRINT("[GET_REP] Content-Range: %s\n", range);
  return range_si;
}


static void _add_common_headers(httpmsg_t *rep,
                                const char *ctype,
                                const httpcache_t *cd,
                                const httpcfg_t *cfg)
{
  msg_add_header(rep, "Server", SVR_VERSION);
  msg_add_header(rep, "Connection", "Keep-Alive");
  msg_add_header(rep, "Accept-Ranges", "bytes");

  /* response - cache control */
  char len_str[16];
  char max_age[16];
  itos((unsigned char *)len_str, cfg->max_age, 10, ' ');
  char *ret = strbld(max_age, "max-age=");
  ret = strbld(ret, len_str);
  *ret++ = '\0';
  //D_PRINT("[CACHE] %s\n", max_age);
  msg_add_header(rep, "Cache-Control", max_age);

  char rep_date[30]; /* reply start date */
  time_t rep_time = time(NULL);
  gmt_date(rep_date, &rep_time);
  msg_add_header(rep, "Date", rep_date);
  /* ETag is a strong validator */
  msg_add_header(rep, "ETag", cd->etag);
  msg_add_header(rep, "Last-Modified", cd->last_modified);
  msg_add_header(rep, "Content-Type", ctype);
}

static int _cache_altered(httpmsg_t *rep,
                          const httpcache_t *cd,
                          const httpmsg_t *req)
{
  char *cache_ctl = msg_header_value(req, "Cache-Control");
  if (!cache_ctl) return 1;  /* no cache control */

  if (strstr(cache_ctl, "no-store")) {
    D_PRINT("[CACHE-REQ] Request no-store\n");
    return 1;  /* no cache control */
  }
  if (strstr(cache_ctl, "no-cache")) {
    D_PRINT("[CACHE-REQ] Request no-cache\n");
    return 1;  /* no cache control */
  }

  if (strstr(cache_ctl, "max-age")) {
    char *max_age = split_kv(cache_ctl, '=');
    if (max_age[0] == '0') {
      D_PRINT("[CACHE-REQ] max-age = %s\n", max_age);
      /* req: etag, last_modified */
      char *etag = msg_header_value(req, "If-None-Match");
      if (etag) {
        if (strcmp(cd->etag, etag) == 0) {
          D_PRINT("[CACHE-REQ] etag = %s\n", etag);
          return 0;  /* not modified */
        }
      }
      else {
        char *last_modified = msg_header_value(req, "If-Modified-Since");
        if (last_modified) {
          if (strcmp(cd->last_modified, last_modified) == 0) {
            D_PRINT("[CACHE-REQ] last_modified = %s\n", last_modified);
            return 0;  /* not modified */
          }
        }
      }
    }
  }
  return 1;  /* no cache control or modified */
}

static httpmsg_t *_compressed_rep(char *range,
                                  const char *ctype,
                                  const httpcache_t *cd,
                                  const httpcfg_t *cfg,
                                  const httpmsg_t *req)
{
  char len_str[16];
  httpmsg_t *rep;

  if (!range) {
    if (!_cache_altered(rep, cd, req)) {
      return _304_not_modified();
    }

    /* todo: Cache-Control's other situations should be considered... */
    rep = msg_new();
    msg_set_rep_line(rep, 1, 1, 200, "OK");
    msg_set_body_start(rep, cd->body_zipped);
    itos((unsigned char *)len_str, cd->len_zipped, 10, ' ');
    msg_add_header(rep, "Content-Length", len_str);
    //D_PRINT("[GET_REP] Content-Length = %s\n", len_str);
  }
  else {
    size_t range_s;
    size_t len_range;
    rep = msg_new();
    msg_set_rep_line(rep, 1, 1, 206, "Partial Content");
    range_s = _process_range(rep, range, &len_range, cd->len_zipped);
    //D_PRINT("[GET_REP] range start: %ld, length: %ld\n", range_s, len_range);
    msg_set_body_start(rep, cd->body_zipped + range_s);
    itos((unsigned char *)len_str, len_range, 10, ' ');
    msg_add_header(rep, "Content-Length", len_str);
  }

  msg_add_header(rep, "Content-Encoding", "deflate");
  msg_add_header(rep, "Vary", "Accept-Encoding");
  msg_add_zipped_body(rep, cd->body_zipped, cd->len_zipped);
  _add_common_headers(rep, ctype, cd, cfg);
  return rep;
}

static httpmsg_t *_uncompressed_rep(char *range,
                                    const char *ctype,
                                    const httpcache_t *cd,
                                    const httpcfg_t *cfg,
                                    const httpmsg_t *req)
{
  char len_str[16];
  httpmsg_t *rep;

  if (!range) {
    if (!_cache_altered(rep, cd, req)) {
      return _304_not_modified();
    }

    rep = msg_new();
    msg_set_rep_line(rep, 1, 1, 200, "OK");
    msg_set_body_start(rep, cd->body);
    itos((unsigned char *)len_str, cd->len_body, 10, ' ');
    msg_add_header(rep, "Content-Length", len_str);
  }
  else {
    size_t range_s;
    size_t len_range;
    rep = msg_new();
    msg_set_rep_line(rep, 1, 1, 206, "Partial Content");
    range_s = _process_range(rep, range, &len_range, cd->len_body);
    //D_PRINT("[GET_REP] range start: %ld, length: %ld\n", range_s, len_range);
    msg_set_body_start(rep, cd->body + range_s);
    itos((unsigned char *)len_str, len_range, 10, ' ');
    msg_add_header(rep, "Content-Length", len_str);
  }

  msg_add_body(rep, cd->body, cd->len_body);
  _add_common_headers(rep, ctype, cd, cfg);
  return rep;
}

static httpmsg_t * _prepare_rep(const char *ctype,
                                const int mtype,
                                const httpcache_t *cd,
                                const httpcfg_t *cfg,
                                const httpmsg_t *req)
{
  char *range_str = msg_header_value(req, "Range");
  /* compressed */
  char *zip_enc = msg_header_value(req, "Accept-Encoding");
  //D_PRINT("[PARSER] zip_enc = %s\n", zip_enc);
  if (mtype == MIME_TXT && zip_enc && strstr(zip_enc, "deflate"))
    return _compressed_rep(range_str, ctype, cd, cfg, req);
  else
    return _uncompressed_rep(range_str, ctype, cd, cfg, req);
}

static void _read_to_cache(httpcache_t *data,
                           struct stat *sb,
                           const char *path,
                           const char *ospath,
                           const int mime_type)
{
  char *etag = xmalloc(30);
  char *modified = xmalloc(30);
  sprintf(etag, "\"%lu-%lu-%ld\"", sb->st_ino, sb->st_size, sb->st_mtime);
  gmt_date(modified, &sb->st_mtime);
  size_t len_body = sb->st_size;

  unsigned char *body = io_fread(ospath, len_body);
  //D_PRINT("[IO] len_body: %ld\n", len_body);
  //D_PRINT("[IO] body:\n%s\n", (char *)body);

  /* create the new cache data */
  if (mime_type == MIME_TXT) {  /* compressed */
    struct libdeflate_compressor *c = libdeflate_alloc_compressor(9);
    size_t len_zipbuf = libdeflate_deflate_compress_bound(c, len_body);
    unsigned char *body_zipped = xmalloc(len_zipbuf);
    /* compressed body start should sync with body start */
    size_t len_zipped = libdeflate_deflate_compress(c, body, len_body,
                                                    body_zipped, len_zipbuf);

    libdeflate_free_compressor(c);
    //D_PRINT("[DEFLATE] len_zipped: %ld\n", len_zipped);
    httpcache_set(data, xstrdup(path), etag, modified,
                  body, len_body, body_zipped, len_zipped);
  }
  else {  /* uncompressed */
    httpcache_set(data, xstrdup(path), etag, modified,
                  body, len_body, NULL, 0);
  }
}

static httpmsg_t *_get_rep_msg(rbtree_t *cache,
                               char *path,
                               const httpcfg_t *cfg,
                               const httpmsg_t *req)
{
  char curdir[MAX_CWD];
  char ospath[MAX_PATH];
  char *ret;

  if (!getcwd(curdir, MAX_CWD)) {
    D_PRINT("[SYS] Couldn't read %s\n", curdir);
  }

  if ((strcmp(path, "/demo/login.html") == 0 ||
       strcmp(path, "/demo/script/login.js") == 0 ||
       strcmp(path, "/demo/css/login.css") == 0)) {
    ret = strbld(ospath, curdir);
    ret = strbld(ret, path);
    *ret++ = '\0';
  }
  else {
    char *cookie = msg_header_value(req, "Cookie");
    if (!cookie)
      return _401_unauthorized(path, "Not Authorized or login needed!");

    /* authorization check */
    char *token = split_kv(cookie, '=');
    if (token) {
      /* check user's identity... */
      D_PRINT("[COOKIE] %s\n", token);
      int rc = jwt_verify(token);
      if (rc == JWT_EXPIRED) {
        return _401_unauthorized(path, "Token expired, please relogin!");
      }
      else if (rc == JWT_FAILED) {
        return _401_unauthorized(path, "Illegal, please verify yourself!");
      }
      /* authenticated! */
      ret = strbld(ospath, curdir);
      ret = strbld(ret, path);
      *ret++ = '\0';
    }
    else
      /* restricted resource, start authentication */
      return _401_unauthorized(path, "You haven't logged in!");
  }

  struct stat sb;
  /* file does not exist */
  if (stat(ospath, &sb) == -1)
    return _404_not_found(path);
  /* directory not allowed */
  if (S_ISDIR(sb.st_mode))
    return _403_forbidden(path);

  /* check if the body is in the cache */
  int ctype = HTML;  /* default to HTML */
  char *ext = find_ext(path);
  int mime_type = _find_content_type(&ctype, ext);
  httpcache_t cdata;
  cdata.path = path;

  pthread_mutex_lock(&cache->mutex);
  httpcache_t *cd = (httpcache_t *)rbtree_search(cache, &cdata);
  pthread_mutex_unlock(&cache->mutex);

  if (cd) {
    /* data exceeds max-age, refresh it... */
    long cur_time = mstime();
    if (cur_time - cd->stamp >= cfg->max_age) {
      char etag[30];
      sprintf(etag, "\"%lu-%lu-%ld\"", sb.st_ino, sb.st_size, sb.st_mtime);
      if (strcmp(cd->etag, etag) != 0) {
        httpcache_clear(cd);
        _read_to_cache(cd, &sb, path, ospath, mime_type);
        D_PRINT("[CACHE] <%s> reloaded!\n", cd->path);
      }
      else
        D_PRINT("[CACHE] <%s> revalidated!\n", cd->path);
      cd->stamp = cur_time;
    }
    return _prepare_rep(content_type[ctype], mime_type, cd, cfg, req);
  }
  /* not in the cache, create it... */
  cd = httpcache_new();
  _read_to_cache(cd, &sb, path, ospath, mime_type);
  D_PRINT("[CACHE] <%s> added!\n", path);
  pthread_mutex_lock(&cache->mutex);
  rbtree_insert(cache, cd);
  pthread_mutex_unlock(&cache->mutex);
  return _prepare_rep(content_type[ctype], mime_type, cd, cfg, req);
}

void http_get(const int sockfd,
              rbtree_t *cache,
              char *path,
              const httpcfg_t *cfg,
              const httpmsg_t *req)
{
  httpmsg_t *rep = _get_rep_msg(cache, path, cfg, req);
  /* send headers*/
  msg_send_headers(sockfd, rep);
  /* send body */
  msg_send_body(sockfd, rep->body_s, rep->len_body);

  if (rep->code == 401 || rep->code == 403 || rep->code == 404 ||
      rep->code == 302 || rep->code == 304)
    msg_delete(rep, 1);
  else
    msg_delete(rep, 0);
}
