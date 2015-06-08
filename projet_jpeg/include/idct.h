/* Projet C - Sujet JPEG */
#ifndef __IDCT_H__
#define __IDCT_H__

#include <stdint.h>

/*
 * Puts in out[64] the inverse DCT of in[64]
 */
extern void idct_block(int32_t in[64], uint8_t out[64]);

#endif
