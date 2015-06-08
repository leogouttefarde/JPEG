/* Projet C - Sujet JPEG */
#ifndef __IQZZ_H__
#define __IQZZ_H__

#include <stdint.h>

/* 
 * Puts in out[64] the block in[64] of 8x8 pixels read in zigzag inverse 
 * multiplied by the quantification table
 */
extern void iqzz_block (int32_t in[64], int32_t out[64], uint8_t quantif[64]);


#endif

