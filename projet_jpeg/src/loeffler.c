
#include "idct.h"
#include "common.h"


void rotation(double i0, double i1, double *out0, double *out1, uint8_t k, uint8_t n)
{
        const double COS = cos((n * M_PI)/16);
        const double SIN = sin((n * M_PI)/16);

        *out0 = k * (i0 * COS - i1 * SIN);
        *out1 = k * (i1 * COS + i0 * SIN);
}

void butterfly(double i0, double i1, double *out0, double *out1)
{
        *out0 = i0 + i1;
        *out1 = i0 - i1;
}

void swap(double **a, double **b)
{
        double *temp = *a;

        *a = *b;
        *b = temp;
}

void loeffler(double vect[8])
{
        const double SQRT_2 = sqrt(2);
        double next_buf[8];
        double *next = next_buf;


        /* Etape 4 */

        next[0] = vect[0];
        next[1] = vect[4];
        next[2] = vect[2];
        next[3] = vect[6];

        butterfly(vect[1], vect[7], &next[7], &next[4]);

        next[5] = SQRT_2 * vect[3];
        next[6] = SQRT_2 * vect[5];

        swap(&vect, &next);


        /* Etape 3 */

        butterfly(vect[0], vect[1], &next[0], &next[1]);
        butterfly(vect[4], vect[6], &next[4], &next[6]);
        butterfly(vect[7], vect[5], &next[7], &next[5]);

        rotation(vect[2], vect[3], &next[2], &next[3], SQRT_2, 6);

        swap(&vect, &next);


        /* Etape 2 */

        butterfly(vect[0], vect[3], &next[0], &next[3]);
        butterfly(vect[1], vect[2], &next[1], &next[2]);

        rotation(vect[4], vect[7], &next[4], &next[7], 1, 3);
        rotation(vect[5], vect[6], &next[5], &next[6], 1, 1);

        swap(&vect, &next);


        /* Etape 1 */

        butterfly(vect[0], vect[7], &next[0], &next[7]);
        butterfly(vect[1], vect[6], &next[1], &next[6]);
        butterfly(vect[2], vect[5], &next[2], &next[5]);
        butterfly(vect[3], vect[4], &next[3], &next[4]);

        /* Inutile, remet la copie locale du pointeur vect à la bonne adresse */
        //swap(&vect, &next);
}

void idct_block(int32_t in[64], uint8_t out[64])
{
        const uint8_t N = 8;
        double vector[N];
        double matrix[N*N];

        /* Application sur les lignes de la matrice */
        for (uint8_t x = 0; x < N; ++x) {

                for (uint8_t y = 0; y < N; ++y)
                        vector[y] = in[x*N + y];

                loeffler(vector);

                for (uint8_t y = 0; y < N; ++y)
                        matrix[x*N + y] = vector[y];
        }

        /* Application sur les lignes de sa transposée */
        for (uint8_t y = 0; y < N; ++y) {

                for (uint8_t x = 0; x < N; ++x)
                        vector[x] = matrix[x*N + y];

                loeffler(vector);

                /* Avec Loeffler on multiplie par sqrt(2) donc
                 * après chaque application il faut diviser par
                 * sqrt(N) au lieu de sqrt(2N), et donc vu qu'on
                 * l'applique 2 fois içi il faut diviser par sqrt(N)^2 = N */
                for (uint8_t x = 0; x < N; ++x)
                        out[x*N + y] = double2uint8( vector[x]/N + 128. );
        }
}


