/* license: public domain
 * Implementation of SHA1 hash function.
 * Original author:  Steve Reid <sreid@sea-to-sky.net>
 * Contributions by: James H. Brown <jbrown@burgoyne.com>,
 *                   Saul Kravitz <Saul.Kravitz@celera.com>,
 *                   Ralph Giles <giles@ghostscript.com>
 * Modified by WaterJuice retaining Public Domain license */

#ifndef _SHA1_H_
#define _SHA1_H_


#include <stdint.h>


#define SHA1_HASH_SIZE (160 / 8)

/* sha1_context_t - This must be initialised using Sha1Initialised.
 * Do not modify the contents of this structure directly. */
typedef struct {
  uint32_t state[5];
  uint32_t count[2];
  uint8_t buf[64];
} sha1_context_t;

typedef struct {
  uint8_t bytes[SHA1_HASH_SIZE];
} sha1_hash_t;


/* nitialises an SHA1 Context.
 * Use this to initialise/reset a context */
void sha1_init(sha1_context_t *context);

/* Adds data to the SHA1 context.
 * This will process the data and update the internal state of the context.
 * Keep on calling this function until all the data has been added.
 * Then call Sha1Finalise to calculate the hash */
void sha1_update(sha1_context_t *context,
                 void const *buf,
                 uint32_t bufsize);

/* Performs the final calculation of the hash and
 * returns the digest (20 byte buffer containing 160bit hash).
 * After calling this, sha1_finalise must be used to reuse the context */
void sha1_finalise(sha1_context_t *context,
                   sha1_hash_t *digest);

/* Combines sha1_init, sha1_update, and sha1_finalise into one function.
 * Calculates the SHA1 hash of the buffer */
void sha1_calculate(void const *buf,
                    uint32_t bufsize,
                    sha1_hash_t *digest);


#endif
