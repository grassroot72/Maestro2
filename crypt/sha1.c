/* license: public domain
 * Implementation of SHA1 hash function.
 * Original author:  Steve Reid <sreid@sea-to-sky.net>
 * Contributions by: James H. Brown <jbrown@burgoyne.com>,
 *                   Saul Kravitz <Saul.Kravitz@celera.com>,
 *                   Ralph Giles <giles@ghostscript.com>
 * Modified by WaterJuice retaining Public Domain license */

#include <stdio.h>
#include <memory.h>
#include "memcpy_sse2.h"
#include "sha1.h"


/* Decide whether to use the Little-Endian shortcut.
 * If the shortcut is not used then the code will work correctly
 * on either big or little endian, however if we do know it is a
 * little endian architecture, we can speed it up a bit.
 * Note, there are TWO places where USE_LITTLE_ENDIAN_SHORTCUT is used.
 * They MUST be paired together. */
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && \
    (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    /* gcc defines __BYTE_ORDER__ */
    #define USE_LITTLE_ENDIAN_SHORTCUT
#endif

typedef union {
  uint8_t c[64];
  uint32_t l[16];
} char64long16_u;

/* Endian neutral macro for loading 32 bit value
 * from 4 byte array (in big endian form) */
#define LOAD32H(x, y)                           \
        { x = ((uint32_t)((y)[0] & 255)<<24) |  \
              ((uint32_t)((y)[1] & 255)<<16) |  \
              ((uint32_t)((y)[2] & 255)<<8)  |  \
              ((uint32_t)((y)[3] & 255)); }

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/* blk0() and blk() perform the initial expand. */
#ifdef USE_LITTLE_ENDIAN_SHORTCUT
  #define blk0(i) (block->l[i] = (rol(block->l[i],24)&0xFF00FF00) |  \
                                 (rol(block->l[i],8)&0x00FF00FF))
#else
  #define blk0(i) block->l[i]
#endif

#define blk(i) (block->l[i&15] = rol(block->l[(i+13)&15] ^  \
                                     block->l[(i+8)&15] ^   \
                                     block->l[(i+2)&15] ^   \
                                     block->l[i&15],1))

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i)  z += ((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);     \
                         w=rol(w,30);
#define R1(v,w,x,y,z,i)  z += ((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);      \
                         w=rol(w,30);
#define R2(v,w,x,y,z,i)  z += (w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);            \
                         w=rol(w,30);
#define R3(v,w,x,y,z,i)  z += (((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);  \
                         w=rol(w,30);
#define R4(v,w,x,y,z,i)  z += (w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);            \
                         w=rol(w,30);

/* Loads the 128 bits from ByteArray into WordArray,
 * treating ByteArray as big endian data */
#ifdef USE_LITTLE_ENDIAN_SHORTCUT
  #define Load128BitsAsWords(WordArray, ByteArray)  \
          memcpy_fast(WordArray, ByteArray, 64)
#else
  #define Load128BitsAsWords( WordArray, ByteArray )       \
          {                                                \
            uint32_t i;                                    \
            for (i = 0; i < 16; i++) {                     \
              LOAD32H((WordArray)[i], (ByteArray)+(i*4));  \
            }                                              \
          }
#endif

/* Transform
 * Hash a single 512-bit block. This is the core of the algorithm */
static void _transform(uint32_t state[5],
                       uint8_t const buf[64])
{
  uint32_t a;
  uint32_t b;
  uint32_t c;
  uint32_t d;
  uint32_t e;
  uint8_t workspace[64];
  char64long16_u *block = (char64long16_u *)workspace;

  Load128BitsAsWords(block->l, buf);

  /* Copy context->state[] to working vars */
  a = state[0];
  b = state[1];
  c = state[2];
  d = state[3];
  e = state[4];

  /* 4 rounds of 20 operations each. Loop unrolled. */
  R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
  R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
  R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
  R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
  R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
  R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
  R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
  R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
  R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
  R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
  R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
  R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
  R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
  R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
  R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
  R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
  R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
  R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
  R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
  R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);

  /* Add the working vars back into context.state[] */
  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
  state[4] += e;
}

/* nitialises an SHA1 Context.
 * Use this to initialise/reset a context */
void sha1_init(sha1_context_t *context)
{
  /* SHA1 initialisation constants */
  context->state[0] = 0x67452301;
  context->state[1] = 0xEFCDAB89;
  context->state[2] = 0x98BADCFE;
  context->state[3] = 0x10325476;
  context->state[4] = 0xC3D2E1F0;
  context->count[0] = 0;
  context->count[1] = 0;
}

/* Adds data to the SHA1 context.
 * This will process the data and update the internal state of the context.
 * Keep on calling this function until all the data has been added.
 * Then call Sha1Finalise to calculate the hash */
void sha1_update(sha1_context_t *context,
                 void const *buf,
                 uint32_t bufsize)
{
  uint32_t i;
  uint32_t j;

  j = (context->count[0] >> 3) & 63;
  if ((context->count[0] += bufsize << 3) < (bufsize << 3)) {
    context->count[1]++;
  }

  context->count[1] += (bufsize >> 29);
  if ((j + bufsize) > 63) {
    i = 64 - j;
    memcpy_fast(&context->buf[j], buf, i );
    _transform(context->state, context->buf);
    for (; i + 63 < bufsize; i += 64) {
      _transform(context->state, (uint8_t *)buf + i);
    }
    j = 0;
  }
  else {
    i = 0;
  }

  memcpy_fast(&context->buf[j], &((uint8_t *)buf)[i], bufsize - i );
}

/* Performs the final calculation of the hash and
 * returns the digest (20 byte buffer containing 160bit hash).
 * After calling this, sha1_finalise must be used to reuse the context */
void sha1_finalise(sha1_context_t *context,
                   sha1_hash_t *digest)
{
  uint32_t i;
  uint8_t finalcount[8];

  for (i = 0; i < 8; i++) {
    finalcount[i] = (unsigned char)
                    ((context->count[(i >= 4 ? 0 : 1)] >> ((3-(i & 3))*8)) &
                     255);  /* Endian independent */
  }
  sha1_update(context, (uint8_t *)"\x80", 1);
  while ((context->count[0] & 504) != 448) {
    sha1_update(context, (uint8_t *)"\0", 1);
  }

  sha1_update(context, finalcount, 8);  /* Should cause a transform */
  for (i = 0; i < SHA1_HASH_SIZE; i++) {
    digest->bytes[i] = (uint8_t)((context->state[i>>2] >> ((3-(i & 3))*8)) &
                                 255);
  }
}

/* Combines sha1_init, sha1_update, and sha1_finalise into one function.
 * Calculates the SHA1 hash of the buffer */
void sha1_calculate(void const *buf,
                    uint32_t bufsize,
                    sha1_hash_t *digest)
{
  sha1_context_t context;

  sha1_init(&context);
  sha1_update(&context, buf, bufsize);
  sha1_finalise(&context, digest);
}
