
#include "common.h"
#include "downsampler.h"


/* Computes a downsampled pixel */
static inline uint8_t average_pixel(
                uint8_t *in, uint32_t in_index, uint16_t nb_blocks_in_h,
                uint8_t nb_blocks_h, uint8_t nb_blocks_v)
{
        /* Upsampled pixel-line size */
        const uint32_t LINE = BLOCK_DIM * nb_blocks_in_h;
        uint32_t in_pos;
        uint32_t pixel = 0;

        /*
         * Sums all input pixels to
         * compute their average value
         */
        for (uint16_t v = 0; v < nb_blocks_v; ++v) {

                in_pos = in_index;

                /*
                 * Sum all horizontal pixels
                 */
                for (uint16_t h = 0; h < nb_blocks_h; ++h)
                        pixel += in[in_pos++];


                in_index += LINE;
        }

        /* Compute the average pixel */
        pixel /= nb_blocks_h * nb_blocks_v;


        return (uint8_t)pixel;
}

/* Downsamples a whole block */
static inline void downsample_block(
                uint8_t *in, uint32_t in_index, uint16_t nb_blocks_in_h,
                uint8_t *out, uint32_t out_index,
                uint8_t nb_blocks_h, uint8_t nb_blocks_v)
{
        /* Upsampled bloc-line size */
        const uint32_t LINE = nb_blocks_v * BLOCK_DIM * nb_blocks_in_h;
        uint32_t out_pos, in_pos;
        uint8_t pixel;


        /* Downsample each pixel from the bloc */
        for (uint16_t j = 0; j < BLOCK_DIM; ++j) {

                out_pos = out_index;
                in_pos = in_index;

                for (uint16_t i = 0; i < BLOCK_DIM; ++i) {

                        /* Compute each downsampled pixel */
                        pixel = average_pixel(in, in_pos, nb_blocks_in_h,
                                                nb_blocks_h, nb_blocks_v);

                        out[out_pos++] = pixel;

                        in_pos += nb_blocks_h;
                }

                out_index += BLOCK_DIM;
                in_index += LINE;
        }
}

/* Downsamples any input component's blocks */
void downsampler(uint8_t *in,
                uint8_t nb_blocks_in_h, uint8_t nb_blocks_in_v,
                uint8_t *out,
                uint8_t nb_blocks_out_h, uint8_t nb_blocks_out_v)
{
        const uint8_t nb_blocks_h = nb_blocks_in_h / nb_blocks_out_h;
        const uint8_t nb_blocks_v = nb_blocks_in_v / nb_blocks_out_v;

        /* Downsampled bloc-row size */
        const uint32_t OUT_LINE = BLOCK_SIZE * nb_blocks_out_h;

        /* Upsampled bloc-row size */
        const uint32_t IN_LINE = nb_blocks_v * BLOCK_SIZE * nb_blocks_in_h;

        /* Upsampled bloc increment */
        const uint32_t H_SIZE = BLOCK_DIM * nb_blocks_h;

        uint32_t out_pos, out_index = 0;
        uint32_t in_pos, in_index = 0;


        /* Downsample each bloc */
        for (uint16_t y = 0; y < nb_blocks_out_v; ++y) {

                out_pos = out_index;
                in_pos = in_index;

                for (uint16_t x = 0; x < nb_blocks_out_h; ++x) {

                        /* Downsample a bloc */
                        downsample_block(in, in_pos, nb_blocks_in_h, out, out_pos,
                                        nb_blocks_h, nb_blocks_v);

                        out_pos += BLOCK_SIZE;
                        in_pos += H_SIZE;
                }

                out_index += OUT_LINE;
                in_index += IN_LINE;
        }
}

