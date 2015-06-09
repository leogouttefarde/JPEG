
#include "conv.h"
#include "common.h"
#include "library.h"

/* Convert Y / Cb / Cr MCUs to RGB MCUs */
void YCbCr_to_ARGB(uint8_t  *mcu_YCbCr[3], uint32_t *mcu_RGB,
                uint32_t nb_blocks_h, uint32_t nb_blocks_v)
{
        const uint32_t NB_PIXELS = BLOCK_SIZE * nb_blocks_h * nb_blocks_v;

        uint8_t *Y = mcu_YCbCr[0];
        uint8_t *Cb = mcu_YCbCr[1];
        uint8_t *Cr = mcu_YCbCr[2];
        int32_t R, G, B;

        if (Y == NULL || Cb == NULL || Cr == NULL) {
                printf("ERROR : corrupt YCbCr data\n");
                return;
        }

        /* Convert MCUs to RGB using Y / Cb / Cr MCUs */
        for (uint32_t i = 0; i < NB_PIXELS; ++i) {

                /* Least accurate version (subject p15) */
                /*
                 * R = Y[i] + 1.402 * (Cr[i] - 128);
                 * G = Y[i] - 0.34414 * (Cb[i] - 128) - 0.71414 * (Cr[i] - 128);
                 * B = Y[i] + 1.772 * (Cb[i] - 128);
                 */

                /* Most accurate version (subject p15) */
                R = Y[i] - 0.0009267 * (Cb[i] - 128) + 1.4016868 * (Cr[i] - 128);
                G = Y[i] - 0.3436954 * (Cb[i] - 128) - 0.7141690 * (Cr[i] - 128);
                B = Y[i] + 1.7721604 * (Cb[i] - 128) + 0.0009902 * (Cr[i] - 128);


                /* Convert each color to uint8_t */
                mcu_RGB[i] = TRUNCATE(R) << 16 | TRUNCATE(G) << 8 | TRUNCATE(B);
        }
}

/*
 * Convert Y MCUs to RGB MCUs
 * Used for grayscale images
 */
void Y_to_ARGB(uint8_t *mcu_Y, uint32_t *mcu_RGB,
                uint32_t nb_blocks_h, uint32_t nb_blocks_v)
{
        const uint32_t NB_PIXELS = BLOCK_SIZE * nb_blocks_h * nb_blocks_v;

        uint8_t gray;

        if (mcu_Y == NULL) {
                printf("ERROR : corrupt YCbCr data\n");
                return;
        }
        
        /* Convert MCUs to RGB using Y MCUs */
        for (uint32_t i = 0; i < NB_PIXELS; ++i) {

                /* Extract RGB values from Y values */
                gray = mcu_Y[i];
                mcu_RGB[i] = gray << 16 | gray << 8 | gray;
        }
}


