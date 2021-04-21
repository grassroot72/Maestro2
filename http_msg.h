/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#ifndef _HTTP_MSG_H_
#define _HTTP_MSG_H_


#define SVR_VERSION "Maestro/1.0"

#define METHOD_HEAD 0
#define METHOD_GET 1
#define METHOD_POST 2


typedef struct {
  int method;
  char *path;
  int ver_major;
  int ver_minor;
  int code;      /* status code */
  char *status;  /* status text */

  sllist_t *headers;

  int len_startline;
  int len_headers;

  unsigned char *body;    /* point to the body, raw or compressed */
  unsigned char *body_zipped;
  unsigned char *body_s;  /* point to the range start of the body */
  size_t len_body;
} httpmsg_t;


httpmsg_t *msg_new();

void msg_delete(httpmsg_t *msg,
                const int delbody);

char *msg_header_value(const httpmsg_t *msg,
                       char *key);

int msg_parse(sllist_t *headers,
              unsigned char **startline,
              unsigned char **body,
              size_t *len_body,
              const unsigned char *buf);

void msg_add_header(httpmsg_t *msg,
                    const char *key,
                    const char *value);

int msg_headers_len(const httpmsg_t *msg);

/*------------------------- request header -----------------------------------*/
void msg_set_req_line(httpmsg_t *msg,
                      const char *method,
                      const char *path,
                      const int major,
                      const int minor);

void msg_req_headers(char *headerbytes,
                     const httpmsg_t *req);

/*------------------------- response header ----------------------------------*/
void msg_set_rep_line(httpmsg_t *msg,
                      const int major,
                      const int minor,
                      const int code,
                      const char *status);

void msg_rep_headers(char *headerbytes,
                     const httpmsg_t *rep);

/*---------------------------- message body ----------------------------------*/
void msg_set_body_start(httpmsg_t *msg,
                        unsigned char *s);

void msg_add_body(httpmsg_t *msg,
                  unsigned char *body,
                  const size_t len);

void msg_add_zipped_body(httpmsg_t *msg,
                         unsigned char *body_zipped,
                         const size_t len);

/*---------------------- message send ----------------------------------------*/
void msg_send_headers(const int sockfd,
                      const httpmsg_t *msg);

void msg_send_body(const int sockfd,
                   const unsigned char *data,
                   const int len_data);

void msg_send_body_chunk(const int sockfd,
                         const char *chunk,
                         const int len_chunk);

void msg_send_body_end_chunk(const int sockfd);


#endif
