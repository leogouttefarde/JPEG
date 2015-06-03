/* Projet C - Sujet JPEG */
#ifndef __IDCT_H__
#define __IDCT_H__

#include <stdint.h>

extern void idct_block(int32_t in[64], uint8_t out[64]);

extern void dct_block(uint8_t in[64], int32_t out[64]);

#endif
