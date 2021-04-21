/* license: public domain
 * Implementation of SHA256 hash function.
 * Original author: Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 * Modified by WaterJuice retaining Public Domain license */

#ifndef _SHA256_H_
#define _SHA256_H_


#include <stdint.h>


#define SHA256_HASH_SIZE (256 / 8)

typedef struct {
  uint64_t length;
  uint32_t state[8];
  uint32_t curlen;
  uint8_t buf[64];
} sha256_context_t;

typedef struct {
  uint8_t bytes[SHA256_HASH_SIZE];
} sha256_hash_t;

/* Initialises a SHA256 Context.
 * Use this to initialise/reset a context */
void sha256_init(sha256_context_t *context);

/* Adds data to the SHA256 context.
 * This will process the data and update the internal state of the context.
 * Keep on calling this function until all the data has been added.
 * Then call Sha256Finalise to calculate the hash */
void sha256_update(sha256_context_t *context,
                   void const *buffer,
                   uint32_t bufsize);

/* Performs the final calculation of the hash and returns the digest
 * (32 byte buffer containing 256bit hash). After calling this, sha256_init
 * must be used to reuse the context. */
void sha256_finalise(sha256_context_t *context,
                     sha256_hash_t *digest);

/* Combines sha256_init, sha256_update, and sha256_finalise into one function.
 * Calculates the SHA256 hash of the buffer. */
void sha256_calculate(void const *buf,
                      uint32_t bufsize,
                      sha256_hash_t *digest);


#endif
