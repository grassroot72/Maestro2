/* license: MIT
 * Originally written by https://github.com/h5p9sl
 * modified by Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include <string.h>
#include "memcpy_sse2.h"
#include "sha256.h"
#include "hmac_sha256.h"


#define SIZEOFARRAY(x) sizeof(x) / sizeof(x[0])
#define SHA256_BLOCK_SIZE 64


static void _sha256(const void *data,
                    const unsigned datalen,
                    void *out)
{
  sha256_context_t ctx;
  sha256_hash_t hash;

  sha256_init(&ctx);
  sha256_update(&ctx, data, datalen);
  sha256_finalise(&ctx, &hash);

  memcpy_fast(out, hash.bytes, SHA256_HASH_SIZE);
}

/* concatenate src & dest then sha2 digest them */
static void _concat_and_hash(const void *dest,
                             const unsigned destlen,
                             const void *src,
                             const unsigned srclen,
                             void *out,
                             const unsigned outlen)
{
  uint8_t buf[destlen + srclen];
  uint8_t hash[SHA256_HASH_SIZE];

  memcpy_fast(buf, dest, destlen);
  memcpy_fast(buf + destlen, src, srclen);

  /* Hash 'buf' and store into into another buffer */
  _sha256(buf, SIZEOFARRAY(buf), hash);

  /* Copy the resulting hash to the output buffer
   * Truncate hash if needed */
  //unsigned sz = (SHA256_HASH_SIZE <= outlen) ? SHA256_HASH_SIZE : outlen;
  memcpy_fast(out, hash, SHA256_HASH_SIZE);
}

void hmac_sha256(const void *key,
                 const unsigned keylen,
                 const void *data,
                 const unsigned datalen,
                 void *out, const unsigned outlen)
{
  uint8_t k[SHA256_BLOCK_SIZE]; /* block-sized key derived from 'key' */
  uint8_t k_ipad[SHA256_BLOCK_SIZE];
  uint8_t k_opad[SHA256_BLOCK_SIZE];
  uint8_t hash0[SHA256_HASH_SIZE];
  uint8_t hash1[SHA256_HASH_SIZE];
  int i;

  /* Fill 'k' with zero bytes */
  memset(k, 0, SIZEOFARRAY(k));
  if (keylen > SHA256_BLOCK_SIZE) {
    /* If the key is larger than the hash algorithm's block size,
     * we must digest it first. */
    _sha256(key, keylen, k);
  }
  else {
    memcpy_fast(k, key, keylen);
  }

  /* Create outer & inner padded keys */
  memset(k_ipad, 0x36, SHA256_BLOCK_SIZE);
  memset(k_opad, 0x5c, SHA256_BLOCK_SIZE);
  for (i = 0; i < SHA256_BLOCK_SIZE; i++) {
    k_ipad[i] ^= k[i];
    k_opad[i] ^= k[i];
  }

  /* Perform HMAC algorithm H(K XOR opad, H(K XOR ipad, text))
   * https://tools.ietf.org/html/rfc2104 */
  _concat_and_hash(k_ipad, SIZEOFARRAY(k_ipad),
                   data, datalen,
                   hash0, SIZEOFARRAY(hash0));
  _concat_and_hash(k_opad, SIZEOFARRAY(k_opad),
                   hash0, SIZEOFARRAY(hash0),
                   hash1, SIZEOFARRAY(hash1));

  /* Copy the resulting hash the output buffer
   * Trunacate sha256 hash if needed */
  unsigned sz = (SHA256_HASH_SIZE <= outlen) ? SHA256_HASH_SIZE : outlen;
  memcpy_fast(out, hash1, sz);
}
