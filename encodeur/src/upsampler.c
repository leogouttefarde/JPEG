
#include "upsampler.h"
#include "common.h"


/* Upsamples a pixel */
static inline void upsample_pixel(uint8_t *in, uint32_t in_pos,
                    uint8_t *out, uint32_t out_index, uint16_t nb_blocks_out_h,
                    uint8_t nb_blocks_h, uint8_t nb_blocks_v)
{
        /* Upsampled pixel-line size */
        const uint32_t LINE = BLOCK_DIM * nb_blocks_out_h;

	/* 
	 * Copies the downsampled pixel as many times as required to Upsample a pixel
	 */
        for (uint16_t v = 0; v < nb_blocks_v; ++v) {
                memset(&out[out_index], in[in_pos], nb_blocks_h);
                out_index += LINE;
        }
}

/* Upsamples a whole block */
static inline void upsample_block(uint8_t *in, uint32_t in_index,
                    uint8_t *out, uint32_t out_index, uint16_t nb_blocks_out_h,
                    uint8_t nb_blocks_h, uint8_t nb_blocks_v)
{
        /* Upsampled bloc-line size */
        const uint32_t LINE = nb_blocks_v * BLOCK_DIM * nb_blocks_out_h;
        uint32_t in_pos, out_pos;


        /* Upsample each pixel in the bloc */
        for (uint16_t j = 0; j < BLOCK_DIM; ++j) {

                in_pos = in_index;
                out_pos = out_index;

                for (uint16_t i = 0; i < BLOCK_DIM; ++i) {
			
			/* Upsample a pixel */
                        upsample_pixel(in, in_pos++, out, out_pos, nb_blocks_out_h,
                                        nb_blocks_h, nb_blocks_v);
                        out_pos += nb_blocks_h;
                }

                in_index += BLOCK_DIM;
                out_index += LINE;
        }
}

/* Upsamples any input component's blocks */
void upsampler(uint8_t *in,
                uint8_t nb_blocks_in_h, uint8_t nb_blocks_in_v,
                uint8_t *out,
                uint8_t nb_blocks_out_h, uint8_t nb_blocks_out_v)
{
        const uint8_t nb_blocks_h = nb_blocks_out_h / nb_blocks_in_h;
        const uint8_t nb_blocks_v = nb_blocks_out_v / nb_blocks_in_v;

        /* Downsampled bloc-row size */
        const uint32_t IN_LINE = BLOCK_SIZE * nb_blocks_in_h;

        /* Upsampled bloc-row size */
        const uint32_t OUT_LINE = nb_blocks_v * BLOCK_SIZE * nb_blocks_out_h;

        /* Upsampled bloc increment */
        const uint32_t H_SIZE = BLOCK_DIM * nb_blocks_h;

        uint32_t in_pos, in_index = 0;
        uint32_t out_pos, out_index = 0;


        /* Upsample each bloc */
        for (uint16_t y = 0; y < nb_blocks_in_v; ++y) {

                in_pos = in_index;
                out_pos = out_index;

                for (uint16_t x = 0; x < nb_blocks_in_h; ++x) {
			
			/* Upsample a bloc */
                        upsample_block(in, in_pos, out, out_pos, nb_blocks_out_h,
                                        nb_blocks_h, nb_blocks_v);

                        in_pos += BLOCK_SIZE;
                        out_pos += H_SIZE;
                }

                in_index += IN_LINE;
                out_index += OUT_LINE;
        }
}

