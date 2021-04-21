/* license: MIT license
 * Copyright 2020-2021 @bynect
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include "xmalloc.h"
#include "base64.h"


char *base64_enc(const unsigned char *src,
                 const int len,
                 const int pad,
                 int *outlen)
{
  const char encoding_table[64] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  char *out, *ptr;
  unsigned int bits, buff, size;
  int rem, i;

  if (src == NULL) return NULL;

  size = pad ? (((len + 3 - 1) / 3) * 4) : ((len * 4 + 3 - 1) / 3);
  *outlen = size;
  out = xmalloc((size + 1) * sizeof(char));
  ptr = out;

  bits = 0;
  buff = 0;

  for (i = 0; i < len; ++i) {
    buff <<= 8;
    buff += (unsigned int)(*src++);
    bits += 8;
    while (bits >= 6) {
      *ptr++ = encoding_table[(buff >> (bits - 6)) & 0x3f];
      buff &= ~(0x3f << (bits - 6));
      bits -= 6;
    }
  }

  rem = len % 3;

  if (rem == 1) {
    buff <<= 4;
    *ptr++ = encoding_table[buff & 0x3f];
    if (pad) {
      sprintf(ptr, "%s", "==");
      ptr += 2;
    }
  }
  else if (rem == 2) {
    buff <<= 2;
    *ptr++ = encoding_table[buff & 0x3f];
    if (pad) *ptr++ = '=';
  }

  *ptr = 0;
  return out;
}


unsigned char *base64_dec(const char *src,
                          const int len)
{
  const char decoding_table[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  };

  unsigned char *out, *ptr;
  unsigned int bits, buff, size;
  int i;

  if (src == NULL) return NULL;

  size = len / 4 * 3;
  out = xmalloc((size + 1) * sizeof(unsigned char));
  ptr = out;

  bits = 0;
  buff = 0;

  for (i = 0; i < len; ++i) {
    const unsigned char c = *src++;
    const int group = decoding_table[c];

    if (c == -1) return NULL;

    buff <<= 6;
    bits += 6;

    if (group != -1) buff += group;

    if (bits >= 8) {
      if (c != '=') *ptr++ = (buff >> (bits - 8));
      buff &= ~(0xff << (bits - 8));
      bits -= 8;
    }
  }

  *ptr = 0;
  return out;
}

char *base64url_enc(const unsigned char *src,
                    const int len,
                    const int pad,
                    int *outlen)
{
  const char encoding_table[64] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

  char *out, *ptr;
  unsigned int bits, buff, size;
  int rem, i;

  if (src == NULL) return NULL;

  size = pad ? (((len + 3 - 1) / 3) * 4) : ((len * 4 + 3 - 1) / 3);
  *outlen = size;
  out = xmalloc((size + 1) * sizeof(char));
  ptr = out;

  bits = 0;
  buff = 0;

  for (i = 0; i < len; ++i) {
    buff <<= 8;
    buff += (unsigned int)(*src++);
    bits += 8;
    while (bits >= 6) {
      *ptr++ = encoding_table[(buff >> (bits - 6)) & 0x3f];
      buff &= ~(0x3f << (bits - 6));
      bits -= 6;
    }
  }

  rem = len % 3;

  if (rem == 1) {
    buff <<= 4;
    *ptr++ = encoding_table[buff & 0x3f];
    if (pad) {
      sprintf(ptr, "%s", "==");
      ptr += 2;
    }
  }
  else if (rem == 2) {
    buff <<= 2;
    *ptr++ = encoding_table[buff & 0x3f];
    if (pad) *ptr++ = '=';
  }

  *ptr = 0;
  return out;
}


unsigned char *base64url_dec(const char *src,
                             const int len)
{
  const char decoding_table[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, 63,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  };

  unsigned char *out, *ptr;
  unsigned int bits, buff, size;
  int i;

  if (src == NULL) return NULL;

  size = len / 4 * 3;
  out = xmalloc((size + 1) * sizeof(unsigned char));
  ptr = out;

  bits = 0;
  buff = 0;

  for (i = 0; i < len; ++i) {
    const unsigned char c = *src++;
    const int group = decoding_table[c];

    if (c == -1) return NULL;

    buff <<= 6;
    bits += 6;

    if (group != -1) buff += group;

    if (bits >= 8) {
      if (c != '=') *ptr++ = (buff >> (bits - 8));
      buff &= ~(0xff << (bits - 8));
      bits -= 8;
    }
  }

  *ptr = 0;
  return out;
}
