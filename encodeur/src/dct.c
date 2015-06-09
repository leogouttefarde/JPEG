
#include "dct.h"
#include "common.h"
#include "library.h"


/* Computes the C(x) formula (subject p13) */
double C(const uint8_t x)
{
        double res = 1;

        if (x == 0)
                res = M_SQRT1_2;

        return res;
}

/* Computes the S(x,y) formula (subject p13) */
uint8_t S(const uint8_t x, const uint8_t y, int32_t *in)
{
        const uint8_t SQRT_16 = 4;
        double sum = 0;

        for (uint8_t l = 0; l < BLOCK_DIM; ++l) {
                for (uint8_t u = 0; u < BLOCK_DIM; ++u) {
                        sum += C(l) * C(u) *
				cos(((2*x + 1) * l * M_PI)/ (2*BLOCK_DIM)) *
				cos(((2*y + 1) * u * M_PI)/ (2*BLOCK_DIM)) *
				in[l*BLOCK_DIM + u];
                }
        }

        sum /= SQRT_16;
        sum += 128.;

        return TRUNCATE(sum);
}

/* Computes the inverse discrete cosine transform */
void idct_block(int32_t in[64], uint8_t out[64])
{
        for (uint8_t x = 0; x < BLOCK_DIM; ++x) {
                for (uint8_t y = 0; y < BLOCK_DIM; ++y) {
                        out[x*BLOCK_DIM + y] = S(x, y, in);
                }
        }
}

/* 
 * Computes the S(x,y) formula for DCT
 * http://fr.wikipedia.org/wiki/JPEG#Transform.C3.A9e_DCT
 */
double S_dct(const uint8_t i, const uint8_t j, uint8_t *in)
{
        double sum = 0;

        for (uint8_t x = 0; x < BLOCK_DIM; x++) {
                for (uint8_t y = 0; y < BLOCK_DIM; y++) {
                        sum +=  cos(((2*x + 1) * i * M_PI)/ (2*BLOCK_DIM)) *
                                cos(((2*y + 1) * j * M_PI)/ (2*BLOCK_DIM)) *
                                (in[x*BLOCK_DIM + y]-128);
                }
        }

        return sum;
}

/* Computes the discrete cosine transform */
void dct_block(uint8_t in[64], int32_t out[64])
{
        for (uint8_t i = 0; i < BLOCK_DIM; i++) {
                for (uint8_t j = 0; j < BLOCK_DIM; j++) {
                        out[i*BLOCK_DIM + j] = (int32_t)(C(i)*C(j)*S_dct(i, j, in)/4);
                }
        }
}
