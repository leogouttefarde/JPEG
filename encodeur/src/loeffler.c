
#include "idct.h"
#include "common.h"
#include "library.h"


static inline void rotation_idct(double i0, double i1, double *out0, double *out1, uint8_t k, uint8_t n)
{
        const double COS = cos((n * M_PI)/16);
        const double SIN = sin((n * M_PI)/16);

        *out0 = k * (i0 * COS - i1 * SIN);
        *out1 = k * (i1 * COS + i0 * SIN);
}

static inline void rotation_dct(double i0, double i1, double *out0, double *out1, uint8_t k, uint8_t n)
{
        const double COS = cos((n * M_PI)/16);
        const double SIN = sin((n * M_PI)/16);

        *out0 = k * (i0 * COS + i1 * SIN);
        *out1 = k * (i1 * COS - i0 * SIN);
}

static inline void swap(double **a, double **b)
{
        double *temp = *a;

        *a = *b;
        *b = temp;
}

static inline void butterfly(double i0, double i1, double *out0, double *out1)
{
        *out0 = i0 + i1;
        *out1 = i0 - i1;
}

static inline void loeffler_idct(double vect[BLOCK_DIM])
{
        double next_buf[BLOCK_DIM];
        double *next = next_buf;


        /* Etape 4 */

        next[0] = vect[0];
        next[1] = vect[4];
        next[2] = vect[2];
        next[3] = vect[6];

        butterfly(vect[1], vect[7], &next[7], &next[4]);

        next[5] = M_SQRT2 * vect[3];
        next[6] = M_SQRT2 * vect[5];

        swap(&vect, &next);


        /* Etape 3 */

        butterfly(vect[0], vect[1], &next[0], &next[1]);
        butterfly(vect[4], vect[6], &next[4], &next[6]);
        butterfly(vect[7], vect[5], &next[7], &next[5]);

        rotation_idct(vect[2], vect[3], &next[2], &next[3], M_SQRT2, 6);

        swap(&vect, &next);


        /* Etape 2 */

        butterfly(vect[0], vect[3], &next[0], &next[3]);
        butterfly(vect[1], vect[2], &next[1], &next[2]);

        rotation_idct(vect[4], vect[7], &next[4], &next[7], 1, 3);
        rotation_idct(vect[5], vect[6], &next[5], &next[6], 1, 1);

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
        double vector[BLOCK_DIM];
        double matrix[BLOCK_SIZE];

        /* Application sur les lignes de la matrice */
        for (uint8_t x = 0; x < BLOCK_DIM; ++x) {

                for (uint8_t y = 0; y < BLOCK_DIM; ++y)
                        vector[y] = in[x*BLOCK_DIM + y];

                loeffler_idct(vector);

                for (uint8_t y = 0; y < BLOCK_DIM; ++y)
                        matrix[x*BLOCK_DIM + y] = vector[y];
        }

        /* Application sur les lignes de sa transposée */
        for (uint8_t y = 0; y < BLOCK_DIM; ++y) {

                for (uint8_t x = 0; x < BLOCK_DIM; ++x)
                        vector[x] = matrix[x*BLOCK_DIM + y];

                loeffler_idct(vector);

                /* Avec Loeffler on multiplie par sqrt(2) donc
                 * après chaque application il faut diviser par
                 * sqrt(BLOCK_DIM) au lieu de sqrt(2 * BLOCK_DIM), et donc vu qu'on
                 * l'applique 2 fois içi il faut diviser par sqrt(BLOCK_DIM)^2 = BLOCK_DIM */
                for (uint8_t x = 0; x < BLOCK_DIM; ++x)
                        out[x*BLOCK_DIM + y] = double2uint8( vector[x]/BLOCK_DIM + 128. );
        }
}


static inline void loeffler_dct(double vect[BLOCK_DIM])
{
        double next_buf[BLOCK_DIM];
        double *next = next_buf;


        /* Etape 1 */

        butterfly(vect[0], vect[7], &next[0], &next[7]);
        butterfly(vect[1], vect[6], &next[1], &next[6]);
        butterfly(vect[2], vect[5], &next[2], &next[5]);
        butterfly(vect[3], vect[4], &next[3], &next[4]);

        swap(&vect, &next);


        /* Etape 2 */

        butterfly(vect[0], vect[3], &next[0], &next[3]);
        butterfly(vect[1], vect[2], &next[1], &next[2]);

        rotation_dct(vect[4], vect[7], &next[4], &next[7], 1, 3);
        rotation_dct(vect[5], vect[6], &next[5], &next[6], 1, 1);

        swap(&vect, &next);


        /* Etape 3 */

        butterfly(vect[0], vect[1], &next[0], &next[1]);
        butterfly(vect[4], vect[6], &next[4], &next[6]);
        butterfly(vect[7], vect[5], &next[7], &next[5]);

        rotation_dct(vect[2], vect[3], &next[2], &next[3], M_SQRT2, 6);

        swap(&vect, &next);


        /* Etape 4 */

        next[0] = vect[0];
        next[4] = vect[1];
        next[2] = vect[2];
        next[6] = vect[3];

        butterfly(vect[7], vect[4], &next[1], &next[7]);

        next[3] = M_SQRT2 * vect[5];
        next[5] = M_SQRT2 * vect[6];

        /* Inutile, remet la copie locale du pointeur vect à la bonne adresse */
        //swap(&vect, &next);
}

void dct_block(uint8_t in[64], int32_t out[64])
{
        double vector[BLOCK_DIM];
        double matrix[BLOCK_SIZE];

        /* Application sur les lignes de la matrice */
        for (uint8_t x = 0; x < BLOCK_DIM; ++x) {

                for (uint8_t y = 0; y < BLOCK_DIM; ++y)
                        vector[y] = (in[x*BLOCK_DIM + y] - 128.);

                loeffler_dct(vector);

                for (uint8_t y = 0; y < BLOCK_DIM; ++y)
                        matrix[x*BLOCK_DIM + y] = vector[y];
        }

        /* Application sur les lignes de la transposée */
        for (uint8_t y = 0; y < BLOCK_DIM; ++y) {

                for (uint8_t x = 0; x < BLOCK_DIM; ++x)
                        vector[x] = matrix[x*BLOCK_DIM + y];

                loeffler_dct(vector);

                /* Avec Loeffler on multiplie par sqrt(2) donc
                 * après chaque application il faut diviser par
                 * sqrt(BLOCK_DIM) au lieu de sqrt(2 * BLOCK_DIM), et donc vu qu'on
                 * l'applique 2 fois içi il faut diviser par sqrt(BLOCK_DIM)^2 = BLOCK_DIM */
                for (uint8_t x = 0; x < BLOCK_DIM; ++x)
                        out[x*BLOCK_DIM + y] = (int32_t)(vector[x] / BLOCK_DIM);
        }
}


