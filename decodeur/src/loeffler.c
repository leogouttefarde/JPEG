
#include "idct.h"
#include "common.h"
#include "library.h"


/*
 * Performs an inverse DCT Loeffler rotation
 */
static inline void rotation_idct(double i0, double i1, double *out0, double *out1, double k, uint8_t n)
{
        const double COS = cos((n * M_PI)/16);
        const double SIN = sin((n * M_PI)/16);

        *out0 = k * (i0 * COS - i1 * SIN);
        *out1 = k * (i1 * COS + i0 * SIN);
}

/*
 * Exchanges too double pointers
 */
static inline void swap(double **a, double **b)
{
        double *temp = *a;

        *a = *b;
        *b = temp;
}

/*
 * Performs a Loeffler butterfly operation
 */
static inline void butterfly(double i0, double i1, double *out0, double *out1)
{
        *out0 = i0 + i1;
        *out1 = i0 - i1;
}

/*
 * Applies the 1D inverse DCT Loeffler algorithm
 */
static inline void loeffler_idct(double vect[BLOCK_DIM])
{
        double next_buf[BLOCK_DIM];
        double *next = next_buf;


        /* Stage 4 */

        butterfly(vect[1], vect[7], &next[7], &next[4]);

        next[6] = M_SQRT2 * vect[5];
        next[5] = M_SQRT2 * vect[3];
        next[3] = vect[6];
        next[2] = vect[2];
        next[1] = vect[4];
        next[0] = vect[0];

        swap(&vect, &next);


        /* Stage 3 */

        butterfly(vect[7], vect[5], &next[7], &next[5]);
        butterfly(vect[4], vect[6], &next[4], &next[6]);
        butterfly(vect[0], vect[1], &next[0], &next[1]);

        rotation_idct(vect[2], vect[3], &next[2], &next[3], M_SQRT2, 6);

        swap(&vect, &next);


        /* Stage 2 */

        butterfly(vect[0], vect[3], &next[0], &next[3]);
        butterfly(vect[1], vect[2], &next[1], &next[2]);

        rotation_idct(vect[4], vect[7], &next[4], &next[7], 1, 3);
        rotation_idct(vect[5], vect[6], &next[5], &next[6], 1, 1);

        swap(&vect, &next);


        /* Stage 1 */

        butterfly(vect[3], vect[4], &next[3], &next[4]);
        butterfly(vect[2], vect[5], &next[2], &next[5]);
        butterfly(vect[1], vect[6], &next[1], &next[6]);
        butterfly(vect[0], vect[7], &next[0], &next[7]);

        /* Useless, restaures the local vect pointer's copy to its correct value */
        //swap(&vect, &next);
}

/*
 * Performs a Loeffler inverse discrete cosine transform
 */
void idct_block(int32_t in[64], uint8_t out[64])
{
        double vector[BLOCK_DIM];
        double matrix[BLOCK_SIZE];

        /* Apply Loeffler on the matrix's lines */
        for (uint8_t x = 0; x < BLOCK_DIM; ++x) {

                for (uint8_t y = 0; y < BLOCK_DIM; ++y)
                        vector[y] = in[x*BLOCK_DIM + y];

                loeffler_idct(vector);

                for (uint8_t y = 0; y < BLOCK_DIM; ++y)
                        matrix[x*BLOCK_DIM + y] = vector[y];
        }

        /* Apply Loeffler on its transposition's lines */
        for (uint8_t y = 0; y < BLOCK_DIM; ++y) {

                for (uint8_t x = 0; x < BLOCK_DIM; ++x)
                        vector[x] = matrix[x*BLOCK_DIM + y];

                loeffler_idct(vector);

                /*
                 * The Loeffler algorithm produces
                 * a result upscaled by sqrt(n),
                 * so since we apply it twice here
                 * we have to downscale it by sqrt(n)^2 = n = BLOCK_DIM
                 */
                for (uint8_t x = 0; x < BLOCK_DIM; ++x)
                        out[x*BLOCK_DIM + y] = TRUNCATE( vector[x]/BLOCK_DIM + 128. );
        }
}


