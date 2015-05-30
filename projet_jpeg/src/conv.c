
#include "conv.h"
#include "common.h"


void YCbCr_to_ARGB(uint8_t  *mcu_YCbCr[3], uint32_t *mcu_RGB,
                uint32_t nb_blocks_h, uint32_t nb_blocks_v)
{
        //printf("YCbCr_to_ARGB :\n");
        // printf("nb_blocks_h = %d\n", nb_blocks_h);
        // printf("nb_blocks_v = %d\n\n", nb_blocks_v);

        // print_block(&mcu_YCbCr[0][256]);
        // print_block(&mcu_YCbCr[1][0]);

        // printf("mcu_YCbCr[0][17] = %d\n", mcu_YCbCr[0][17]);
        // printf("mcu_YCbCr[0][42] = %d\n", mcu_YCbCr[0][42]);
        // printf("mcu_YCbCr[1][17] = %d\n", mcu_YCbCr[1][17]);
        // printf("mcu_YCbCr[1][42] = %d\n", mcu_YCbCr[1][42]);
        // printf("mcu_YCbCr[2][17] = %d\n", mcu_YCbCr[2][17]);
        // printf("mcu_YCbCr[2][42] = %d\n\n", mcu_YCbCr[2][42]);
        // exit(0);

        const uint32_t SIZE = BLOCK_DIM*nb_blocks_h * BLOCK_DIM*nb_blocks_v;

        uint8_t *Y = mcu_YCbCr[0];
        uint8_t *Cb = mcu_YCbCr[1];
        uint8_t *Cr = mcu_YCbCr[2];
        int32_t R, G, B;

        if (Y == NULL || Cb == NULL || Cr == NULL) {
                printf("ERROR : corrupt YCbCr data\n");
                return;
        }

        for (uint32_t i = 0; i < SIZE; ++i) {

                /* Conversion moins précise */
                // R = Y[i] + 1.402 * (Cr[i] - 128);
                // G = Y[i] - 0.34414 * (Cb[i] - 128) - 0.71414 * (Cr[i] - 128);
                // B = Y[i] + 1.772 * (Cb[i] - 128);

                /* Conversion plus précise */
                R = Y[i] - 0.0009267 * (Cb[i] - 128) + 1.4016868 * (Cr[i] - 128);
                G = Y[i] - 0.3436954 * (Cb[i] - 128) - 0.7141690 * (Cr[i] - 128);
                B = Y[i] + 1.7721604 * (Cb[i] - 128) + 0.0009902 * (Cr[i] - 128);


                mcu_RGB[i] = truncate(R) << 16 | truncate(G) << 8 | truncate(B);
        }
}


