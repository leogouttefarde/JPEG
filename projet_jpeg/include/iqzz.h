/* Projet C - Sujet JPEG */
#ifndef __IQZZ_H__
#define __IQZZ_H__

#include <stdint.h>

/*
 * Computes an inverse zigzag quantification.
 */
extern void iqzz_block (int32_t in[64], int32_t out[64], uint8_t quantif[64]);


#endif

