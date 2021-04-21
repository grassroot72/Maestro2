/* license: public domain
 * MD5 hash function Originally written by Alexander Peslyak.
 * Modified by WaterJuice retaining Public Domain license */

#include <stdio.h>
#include <memory.h>
#include "memcpy_sse2.h"
#include "md5.h"


/* F, G, H, I
 * The basic MD5 functions.
 * F and G are optimised compared to their RFC 1321 definitions
 * for architectures that lack an AND-NOT instruction, just like
 * in Colin Plumb's implementation */
#define F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z) ((y) ^ ((z) & ((x) ^ (y))))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | ~(z)))

/* STEP
 * The MD5 transformation for all four rounds */
#define STEP(f, a, b, c, d, x, t, s)                                  \
          (a) += f((b), (c), (d)) + (x) + (t);                        \
          (a) = (((a) << (s)) | (((a) & 0xffffffff) >> (32 - (s))));  \
          (a) += (b);

/* transform
 * This processes one or more 64-byte data blocks,
 * but does NOT update the bit counters.
 * There are no alignment requirements */
static void *_transform(md5_context_t *ctx,
                        void const *data,
                        uintmax_t size)
{
  uint8_t* ptr;
  uint32_t a;
  uint32_t b;
  uint32_t c;
  uint32_t d;
  uint32_t saved_a;
  uint32_t saved_b;
  uint32_t saved_c;
  uint32_t saved_d;

  #define GET(n) (ctx->block[(n)])
  #define SET(n) (ctx->block[(n)] =            \
            ((uint32_t)ptr[(n)*4 + 0] << 0 )   \
          | ((uint32_t)ptr[(n)*4 + 1] << 8 )   \
          | ((uint32_t)ptr[(n)*4 + 2] << 16)   \
          | ((uint32_t)ptr[(n)*4 + 3] << 24))

  ptr = (uint8_t *)data;

  a = ctx->a;
  b = ctx->b;
  c = ctx->c;
  d = ctx->d;

  do {
    saved_a = a;
    saved_b = b;
    saved_c = c;
    saved_d = d;

    /* Round 1 */
    STEP(F, a, b, c, d, SET(0),  0xd76aa478, 7)
    STEP(F, d, a, b, c, SET(1),  0xe8c7b756, 12)
    STEP(F, c, d, a, b, SET(2),  0x242070db, 17)
    STEP(F, b, c, d, a, SET(3),  0xc1bdceee, 22)
    STEP(F, a, b, c, d, SET(4),  0xf57c0faf, 7)
    STEP(F, d, a, b, c, SET(5),  0x4787c62a, 12)
    STEP(F, c, d, a, b, SET(6),  0xa8304613, 17)
    STEP(F, b, c, d, a, SET(7),  0xfd469501, 22)
    STEP(F, a, b, c, d, SET(8 ),  0x698098d8, 7)
    STEP(F, d, a, b, c, SET(9 ),  0x8b44f7af, 12)
    STEP(F, c, d, a, b, SET(10 ), 0xffff5bb1, 17)
    STEP(F, b, c, d, a, SET(11 ), 0x895cd7be, 22)
    STEP(F, a, b, c, d, SET(12 ), 0x6b901122, 7)
    STEP(F, d, a, b, c, SET(13 ), 0xfd987193, 12)
    STEP(F, c, d, a, b, SET(14 ), 0xa679438e, 17)
    STEP(F, b, c, d, a, SET(15 ), 0x49b40821, 22)
    /* Round 2 */
    STEP(G, a, b, c, d, GET(1),  0xf61e2562, 5)
    STEP(G, d, a, b, c, GET(6),  0xc040b340, 9)
    STEP(G, c, d, a, b, GET(11), 0x265e5a51, 14)
    STEP(G, b, c, d, a, GET(0),  0xe9b6c7aa, 20)
    STEP(G, a, b, c, d, GET(5),  0xd62f105d, 5)
    STEP(G, d, a, b, c, GET(10), 0x02441453, 9)
    STEP(G, c, d, a, b, GET(15), 0xd8a1e681, 14)
    STEP(G, b, c, d, a, GET(4),  0xe7d3fbc8, 20)
    STEP(G, a, b, c, d, GET(9),  0x21e1cde6, 5)
    STEP(G, d, a, b, c, GET(14), 0xc33707d6, 9)
    STEP(G, c, d, a, b, GET(3),  0xf4d50d87, 14)
    STEP(G, b, c, d, a, GET(8),  0x455a14ed, 20)
    STEP(G, a, b, c, d, GET(13), 0xa9e3e905, 5)
    STEP(G, d, a, b, c, GET(2),  0xfcefa3f8, 9)
    STEP(G, c, d, a, b, GET(7),  0x676f02d9, 14)
    STEP(G, b, c, d, a, GET(12), 0x8d2a4c8a, 20)
    /* Round 3 */
    STEP(H, a, b, c, d, GET(5),  0xfffa3942, 4)
    STEP(H, d, a, b, c, GET(8),  0x8771f681, 11)
    STEP(H, c, d, a, b, GET(11), 0x6d9d6122, 16)
    STEP(H, b, c, d, a, GET(14), 0xfde5380c, 23)
    STEP(H, a, b, c, d, GET(1),  0xa4beea44, 4)
    STEP(H, d, a, b, c, GET(4),  0x4bdecfa9, 11)
    STEP(H, c, d, a, b, GET(7),  0xf6bb4b60, 16)
    STEP(H, b, c, d, a, GET(10), 0xbebfbc70, 23)
    STEP(H, a, b, c, d, GET(13), 0x289b7ec6, 4)
    STEP(H, d, a, b, c, GET(0),  0xeaa127fa, 11)
    STEP(H, c, d, a, b, GET(3),  0xd4ef3085, 16)
    STEP(H, b, c, d, a, GET(6),  0x04881d05, 23)
    STEP(H, a, b, c, d, GET(9),  0xd9d4d039, 4)
    STEP(H, d, a, b, c, GET(12), 0xe6db99e5, 11)
    STEP(H, c, d, a, b, GET(15), 0x1fa27cf8, 16)
    STEP(H, b, c, d, a, GET(2),  0xc4ac5665, 23)
    /* Round 4 */
    STEP(I, a, b, c, d, GET(0),  0xf4292244, 6)
    STEP(I, d, a, b, c, GET(7),  0x432aff97, 10)
    STEP(I, c, d, a, b, GET(14), 0xab9423a7, 15)
    STEP(I, b, c, d, a, GET(5),  0xfc93a039, 21)
    STEP(I, a, b, c, d, GET(12), 0x655b59c3, 6)
    STEP(I, d, a, b, c, GET(3),  0x8f0ccc92, 10)
    STEP(I, c, d, a, b, GET(10), 0xffeff47d, 15)
    STEP(I, b, c, d, a, GET(1),  0x85845dd1, 21)
    STEP(I, a, b, c, d, GET(8),  0x6fa87e4f, 6)
    STEP(I, d, a, b, c, GET(15), 0xfe2ce6e0, 10)
    STEP(I, c, d, a, b, GET(6),  0xa3014314, 15)
    STEP(I, b, c, d, a, GET(13), 0x4e0811a1, 21)
    STEP(I, a, b, c, d, GET(4),  0xf7537e82, 6)
    STEP(I, d, a, b, c, GET(11), 0xbd3af235, 10)
    STEP(I, c, d, a, b, GET(2),  0x2ad7d2bb, 15)
    STEP(I, b, c, d, a, GET(9),  0xeb86d391, 21)

    a += saved_a;
    b += saved_b;
    c += saved_c;
    d += saved_d;

    ptr += 64;
  } while (size -= 64);

  ctx->a = a;
  ctx->b = b;
  ctx->c = c;
  ctx->d = d;

  #undef GET
  #undef SET
  return ptr;
}

/* Initialises an MD5 Context.
 * Use this to initialise/reset a context */
void md5_init(md5_context_t *context)
{
  context->a = 0x67452301;
  context->b = 0xefcdab89;
  context->c = 0x98badcfe;
  context->d = 0x10325476;
  context->lo = 0;
  context->hi = 0;
}

/* Adds data to the MD5 context.
 * This will process the data and update the internal state of the context.
 * Keep on calling this function until all the data has been added.
 * Then call md5_finalise to calculate the hash */
void md5_update(md5_context_t *context,
                void const *buf,
                uint32_t bufsize)
{
  uint32_t saved_lo;
  uint32_t used;
  uint32_t free;

  saved_lo = context->lo;
  if ((context->lo = (saved_lo + bufsize) & 0x1fffffff) < saved_lo) {
    context->hi++;
  }
  context->hi += (uint32_t)(bufsize >> 29);

  used = saved_lo & 0x3f;

  if (used) {
    free = 64 - used;

    if (bufsize < free) {
      memcpy_fast(&context->buf[used], buf, bufsize);
      return;
    }

    memcpy_fast(&context->buf[used], buf, free);
    buf = (uint8_t*)buf + free;
    bufsize -= free;
    _transform(context, context->buf, 64);
  }

  if (bufsize >= 64 ) {
    buf = _transform(context, buf, bufsize & ~(unsigned long)0x3f);
    bufsize &= 0x3f;
  }

  memcpy_fast(context->buf, buf, bufsize);
}

/* Performs the final calculation of the hash and
 * returns the digest (16 byte buffer containing 128bit hash).
 * After calling this, md5_init must be used to reuse the context */
void md5_finalise(md5_context_t *context,
                  md5_hash_t *digest)
{
  uint32_t used;
  uint32_t free;

  used = context->lo & 0x3f;
  context->buf[used++] = 0x80;
  free = 64 - used;

  if (free < 8) {
    memset(&context->buf[used], 0, free);
    _transform(context, context->buf, 64);
    used = 0;
    free = 64;
  }

  memset(&context->buf[used], 0, free - 8);
  context->lo <<= 3;
  context->buf[56] = (uint8_t)(context->lo);
  context->buf[57] = (uint8_t)(context->lo >> 8);
  context->buf[58] = (uint8_t)(context->lo >> 16);
  context->buf[59] = (uint8_t)(context->lo >> 24);
  context->buf[60] = (uint8_t)(context->hi);
  context->buf[61] = (uint8_t)(context->hi >> 8);
  context->buf[62] = (uint8_t)(context->hi >> 16);
  context->buf[63] = (uint8_t)(context->hi >> 24);

  _transform(context, context->buf, 64);
  digest->bytes[0]  = (uint8_t)(context->a);
  digest->bytes[1]  = (uint8_t)(context->a >> 8);
  digest->bytes[2]  = (uint8_t)(context->a >> 16);
  digest->bytes[3]  = (uint8_t)(context->a >> 24);
  digest->bytes[4]  = (uint8_t)(context->b);
  digest->bytes[5]  = (uint8_t)(context->b >> 8);
  digest->bytes[6]  = (uint8_t)(context->b >> 16);
  digest->bytes[7]  = (uint8_t)(context->b >> 24);
  digest->bytes[8]  = (uint8_t)(context->c);
  digest->bytes[9]  = (uint8_t)(context->c >> 8);
  digest->bytes[10] = (uint8_t)(context->c >> 16);
  digest->bytes[11] = (uint8_t)(context->c >> 24);
  digest->bytes[12] = (uint8_t)(context->d);
  digest->bytes[13] = (uint8_t)(context->d >> 8);
  digest->bytes[14] = (uint8_t)(context->d >> 16);
  digest->bytes[15] = (uint8_t)(context->d >> 24);
}

/* Combines md5_init, md5_update, and md5_finalise into one function
 * Calculates the MD5 hash of the buffer */
void md5_calculate(void const *buf,
                   uint32_t bufsize,
                   md5_hash_t *digest)
{
  md5_context_t context;

  md5_init(&context );
  md5_update(&context, buf, bufsize);
  md5_finalise(&context, digest);
}
