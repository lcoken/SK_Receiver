#ifndef __MD5_H__
#define __MD5_H__

#include "skhl_data_typedef.h"

/* MD5 context. */
typedef struct {
  uint32_t  state[4];                                   /* state (ABCD) */
  uint32_t  count[2];        /* number of bits, modulo 2^64 (lsb first) */
  uint8_t   buffer[64];                         /* input buffer */
} MD5_CTX;

#ifdef __cplusplus
extern "C" {
#endif

void MD5Init(MD5_CTX *);
void MD5Update(MD5_CTX *, unsigned char *, unsigned int);
void MD5Final(unsigned char [16], MD5_CTX *);

#ifdef __cplusplus
}
#endif

#endif
