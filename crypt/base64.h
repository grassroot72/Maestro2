/* license: MIT license
 * Copyright 2020-2021 @bynect
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#ifndef _BASE64_H_
#define _BASE64_H_


char *base64_enc(const unsigned char *src,
                 const int len,
                 const int pad,
                 int *outlen);

unsigned char *base64_dec(const char *src,
                          const int len);

char *base64url_enc(const unsigned char *src,
                    const int len,
                    const int pad,
                    int *outlen);

unsigned char *base64url_dec(const char *src,
                             const int len);


#endif
