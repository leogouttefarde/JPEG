
#include "upsampler.h"


/*
 * Optimized equations :
 *      us = h + v * 8 * nb_block_H + i * h_factor + j * v_factor * 8 * nb_block_H + x * 8 * h_factor + y * v_factor * 8 * nb_block_H * 8;
 *      ds = i + j * 8 + x * 64 + y * 64 * nb_block_H / h_factor;
 */
// memcpy
static inline void upsample_pixel(uint8_t *in, uint8_t *out,
                    uint32_t in_pos, uint32_t out_index,
                    uint8_t nb_blocks_h, uint8_t nb_blocks_v,
                    uint16_t nb_blocks_out_h)
{
        const uint32_t LINE = 8 * nb_blocks_out_h;
        uint32_t out_pos;

        for (uint16_t v = 0; v < nb_blocks_v; ++v) {

                out_pos = out_index;

                for (uint16_t h = 0; h < nb_blocks_h; ++h) {

                        out[out_pos++] = in[in_pos];
                }

                out_index += LINE;
        }
}

static inline void upsample_block(uint8_t *in, uint8_t *out,
                    uint32_t in_index, uint32_t out_index,
                    uint8_t nb_blocks_h, uint8_t nb_blocks_v,
                    uint16_t nb_blocks_out_h)
{
        const uint32_t LINE = nb_blocks_v * 8 * nb_blocks_out_h;
        uint32_t in_pos, out_pos;

        for (uint16_t j = 0; j < 8; ++j) {

                in_pos = in_index;
                out_pos = out_index;

                for (uint16_t i = 0; i < 8; ++i) {

                        upsample_pixel(in, out, in_pos++, out_pos, nb_blocks_h, nb_blocks_v, nb_blocks_out_h);
                        out_pos += nb_blocks_h;
                }

                in_index += 8;
                out_index += LINE;
        }
}

void upsampler(uint8_t *in,
                uint8_t nb_blocks_in_h, uint8_t nb_blocks_in_v,
                uint8_t *out,
                uint8_t nb_blocks_out_h, uint8_t nb_blocks_out_v)
{
        const uint8_t nb_blocks_v = nb_blocks_out_v / nb_blocks_in_v;
        const uint8_t nb_blocks_h = nb_blocks_out_h / nb_blocks_in_h;

        /* Optimizations */
        const uint32_t IN_LINE = 64 * nb_blocks_in_h;
        const uint32_t OUT_LINE = nb_blocks_v * 64 * nb_blocks_out_h;
        const uint32_t H_SIZE = 8 * nb_blocks_h;
        uint32_t in_pos, in_index = 0;
        uint32_t out_pos, out_index = 0;


        for (uint16_t y = 0; y < nb_blocks_in_v; ++y) {

                in_pos = in_index;
                out_pos = out_index;

                for (uint16_t x = 0; x < nb_blocks_in_h; ++x) {

                        upsample_block(in, out, in_pos, out_pos, nb_blocks_h, nb_blocks_v, nb_blocks_out_h);

                        in_pos += 64;
                        out_pos += H_SIZE;
                }

                in_index += IN_LINE;
                out_index += OUT_LINE;
        }
}

