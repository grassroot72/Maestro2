/* license: MIT
 * Originally written by https://github.com/h5p9sl
 * modified by Edward LEI <edward_lei72@hotmail.com> */

#ifndef _HMAC_SHA256_H_
#define _HMAC_SHA256_H_


#define HMAC_HASH_SIZE 32

/* key - Should be at least 32 bytes long for optimal security.
 * data - to hash along with the key.
 * out - The output hash.
 *       Should be 32 bytes long, but if it's less than 32 bytes,
 *       the function will truncate the resulting hash */
void hmac_sha256(const void *key,
                 const unsigned keylen,
                 const void *data,
                 const unsigned datalen,
                 void *out, const unsigned outlen);

#endif
