
#include "upsampler.h"
#include "common.h"


/*
 * Optimized equations :
 *      out_pos = h + v * 8 * nb_blocks_out_h + i * nb_blocks_h + j * nb_blocks_v * 8 * nb_blocks_out_h + x * 8 * nb_blocks_h + y * nb_blocks_v * 8 * nb_blocks_out_h * 8;
 *      int_pos = i + j * 8 + x * 64 + y * 64 * nb_blocks_out_h / nb_blocks_h;
 */
static inline void upsample_pixel(uint8_t *in, uint32_t in_pos,
                    uint8_t *out, uint32_t out_index, uint16_t nb_blocks_out_h,
                    uint8_t nb_blocks_h, uint8_t nb_blocks_v)
{
        const uint32_t LINE = BLOCK_DIM * nb_blocks_out_h;
        uint32_t out_pos;

        for (uint16_t v = 0; v < nb_blocks_v; ++v) {

                out_pos = out_index;


                /*
                 * Copie optimale
                 */
                memset(&out[out_index], in[in_pos], sizeof(uint8_t) * nb_blocks_h);

                /*
                 * Copie non optimale
                 */
                /*for (uint16_t h = 0; h < nb_blocks_h; ++h)
                        out[out_pos++] = in[in_pos];*/


                out_index += LINE;
        }
}

static inline void upsample_block(uint8_t *in, uint32_t in_index,
                    uint8_t *out, uint32_t out_index, uint16_t nb_blocks_out_h,
                    uint8_t nb_blocks_h, uint8_t nb_blocks_v)
{
        const uint32_t LINE = nb_blocks_v * BLOCK_DIM * nb_blocks_out_h;
        uint32_t in_pos, out_pos;

        for (uint16_t j = 0; j < BLOCK_DIM; ++j) {

                in_pos = in_index;
                out_pos = out_index;

                for (uint16_t i = 0; i < BLOCK_DIM; ++i) {

                        upsample_pixel(in, in_pos++, out, out_pos, nb_blocks_out_h,
                                        nb_blocks_h, nb_blocks_v);
                        out_pos += nb_blocks_h;
                }

                in_index += BLOCK_DIM;
                out_index += LINE;
        }
}

void upsampler(uint8_t *in,
                uint8_t nb_blocks_in_h, uint8_t nb_blocks_in_v,
                uint8_t *out,
                uint8_t nb_blocks_out_h, uint8_t nb_blocks_out_v)
{
        const uint8_t nb_blocks_h = nb_blocks_out_h / nb_blocks_in_h;
        const uint8_t nb_blocks_v = nb_blocks_out_v / nb_blocks_in_v;

        /* Optimizations */
        const uint32_t IN_LINE = BLOCK_SIZE * nb_blocks_in_h;
        const uint32_t OUT_LINE = nb_blocks_v * BLOCK_SIZE * nb_blocks_out_h;
        const uint32_t H_SIZE = BLOCK_DIM * nb_blocks_h;
        uint32_t in_pos, in_index = 0;
        uint32_t out_pos, out_index = 0;


        for (uint16_t y = 0; y < nb_blocks_in_v; ++y) {

                in_pos = in_index;
                out_pos = out_index;

                for (uint16_t x = 0; x < nb_blocks_in_h; ++x) {

                        upsample_block(in, in_pos, out, out_pos, nb_blocks_out_h,
                                        nb_blocks_h, nb_blocks_v);

                        in_pos += BLOCK_SIZE;
                        out_pos += H_SIZE;
                }

                in_index += IN_LINE;
                out_index += OUT_LINE;
        }
}

