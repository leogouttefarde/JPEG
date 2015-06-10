/* Projet C - Sujet JPEG */
#ifndef __IDCT_H__
#define __IDCT_H__

#include <stdint.h>

/*
 * Computes the inverse discrete cosine transform
 */
extern void idct_block(int32_t in[64], uint8_t out[64]);

#endif
