
#include "idct.h"
#include "common.h"


double C(const uint8_t x)
{
        double res = 1;

        if (x == 0)
                res = M_SQRT1_2;

        return res;
}

uint8_t S(const uint8_t x, const uint8_t y, int32_t *input)
{
        const uint8_t N = 8;
        const uint8_t SQRT_2N = 4;
        double sum = 0;

        for (uint8_t l = 0; l < N; ++l) {
                for (uint8_t u = 0; u < N; ++u) {
                        sum += C(l) * C(u) *
                               cos(((2*x + 1) * l * M_PI)/ (2*N)) *
                               cos(((2*y + 1) * u * M_PI)/ (2*N)) *
                               input[l*N + u];
                }
        }

        sum /= SQRT_2N;
        sum += 128.;

        return double2uint8(sum);
}

void idct_block(int32_t in[64], uint8_t out[64])
{
        const uint8_t N = 8;

        for (uint8_t x = 0; x < N; ++x) {
                for (uint8_t y = 0; y < N; ++y) {
                        out[x*N + y] = S(x, y, in);
                }
        }
}

