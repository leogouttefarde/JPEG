
#include "qzz.h"
#include "common.h"
#include "library.h"

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

/*
 * Computes a zigzag quantification.
 */
void qzz_block (int32_t in[64], int32_t out[64], uint8_t quantif[64])
{
        uint8_t z = 0;
        for(uint8_t i = 0; i < 64; ++i) {
                z = zz[i];
                out[z] = in[i] / quantif[z];
        }
}

/*
 * Adjusts a quantification table's compression quality.
 *
 * Quality range : 0 - 25
 *
 * 0  : No compression
 * 25 : Max legal compression
 *
 * Quality shouldn't exceed 25 according to JPEG standard :
 * http://www-ljk.imag.fr/membres/Valerie.Perrier/SiteWeb/node10.html
 */
void quantify_qtable(uint8_t out[64], const uint8_t in[64], uint8_t quality)
{
        int32_t q_value;

        for(uint8_t i = 0; i < 64; ++i) {
                q_value = 1 + ((uint32_t)in[i] - 1) * (uint32_t)quality;
                out[i] = TRUNCATE(q_value);
        }
}


