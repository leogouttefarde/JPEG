
#include "idct.h"
#include "common.h"


double C(const uint8_t x)
{
        double res = 1;

        if (x == 0)
                res = M_SQRT1_2;

        return res;
}

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

        return double2uint8(sum);
}

void idct_block(int32_t in[64], uint8_t out[64])
{
        for (uint8_t x = 0; x < BLOCK_DIM; ++x) {
                for (uint8_t y = 0; y < BLOCK_DIM; ++y) {
                        out[x*BLOCK_DIM + y] = S(x, y, in);
                }
        }
}

