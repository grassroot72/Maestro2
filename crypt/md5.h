/* license: public domain
 * MD5 hash function Originally written by Alexander Peslyak.
 * Modified by WaterJuice retaining Public Domain license */

#ifndef _MD5_H_
#define _MD5_H_


#include <stdint.h>


#define MD5_HASH_SIZE (128 / 8)

/* md5_context_t - This must be initialised using md5_init.
 * Do not modify the contents of this structure directly */
typedef struct {
  uint32_t lo;
  uint32_t hi;
  uint32_t a;
  uint32_t b;
  uint32_t c;
  uint32_t d;
  uint8_t buf[64];
  uint32_t block[16];
} md5_context_t;

typedef struct {
  uint8_t bytes[MD5_HASH_SIZE];
} md5_hash_t;


/* Initialises an MD5 Context.
 * Use this to initialise/reset a context */
void md5_init(md5_context_t *context);

/* Adds data to the MD5 context.
 * This will process the data and update the internal state of the context.
 * Keep on calling this function until all the data has been added.
 * Then call md5_finalise to calculate the hash */
void md5_update(md5_context_t *context,
                void const *buf,
                uint32_t bufsize);

/* Performs the final calculation of the hash and
 * returns the digest (16 byte buffer containing 128bit hash).
 * After calling this, md5_init must be used to reuse the context */
void md5_finalise(md5_context_t *context,
                  md5_hash_t *digest);

/* Combines md5_init, md5_update, and md5_finalise into one function
 * Calculates the MD5 hash of the buffer */
void md5_calculate(void const *buf,
                   uint32_t bufsize,
                   md5_hash_t *digest);


#endif
