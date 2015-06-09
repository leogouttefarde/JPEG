
#include "iqzz.h"
#include "common.h"


/*
 * Used to optimize zigzag navigation in 8x8 blocks
 */
static const uint8_t zz[64] = 
{
         0,  1,  5,  6, 14, 15, 27, 28,
         2,  4,  7, 13, 16, 26, 29, 42,
         3,  8, 12, 17, 25, 30, 41, 43,
         9, 11, 18, 24, 31, 40, 44, 53,
        10, 19, 23, 32, 39, 45, 52, 54,
        20, 22, 33, 38, 46, 51, 55, 60,
        21, 34, 37, 47, 50, 56, 59, 61,
        35, 36, 48, 49, 57, 58, 62, 63
};

/*
 * Computes an inverse zigzag quantification.
 */
void iqzz_block (int32_t in[64], int32_t out[64], uint8_t quantif[64])
{
        uint8_t z = 0;
        for(uint8_t i = 0; i < 64; ++i) {
                z = zz[i];
                out[i] = in[z] * quantif[z];
        }
}


