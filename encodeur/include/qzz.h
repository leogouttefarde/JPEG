/* Projet C - Sujet JPEG */
#ifndef __IQZZ_H__
#define __IQZZ_H__

#include <stdint.h>

/* 
 * Puts in out[64] the block in[64] of 8x8 pixels read in zigzag inverse 
 * multiplied by the quantification table
 */
extern void iqzz_block (int32_t in[64], int32_t out[64], uint8_t quantif[64]);

/* 
 * Puts in out[64] the block in[64] of 8x8 pixels divided by the 
 * quantification table. 
 * out[64] is written in zigzag.
 */
extern void qzz_block (int32_t out[64], int32_t in[64], uint8_t quantif[64]);

/* 
 * Puts in out[64] the new quantification table related to quality
 */
extern void quantify_qtable(uint8_t out[64], const uint8_t in[64], uint8_t quality);

#endif

