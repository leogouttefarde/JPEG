
#include "pack.h"
#include "common.h"


static inline uint8_t magnitude_class(int16_t value)
{
        uint8_t class = 0;

        if (value < 0)
                value *= -1;

        while (value) {
                value >>= 1;

                class++;
        }

        return class;
}

uint8_t write_magnitude(struct bitstream *stream, int16_t value)
{
        uint8_t bit;
        uint8_t class = magnitude_class(value);

        if (class > 0) {
                if (value < 0)
                        value += (1 << class) - 1;

                for (uint8_t k = 0; k < class; ++k) {
                        bit = (value >> (class - 1 - k)) & 1;
                        write_bit(stream, bit, true);
                }
        }

        return class;
}

void pack_block(struct bitstream *stream,
                struct huff_table *table_DC, int32_t *pred_DC,
                struct huff_table *table_AC,
                int32_t bloc[64], uint32_t **freqs)
{
        uint8_t class, zeros, symbol;
        uint8_t n = 0;
        int16_t diff;

        if ( ((table_AC == NULL || table_DC == NULL) && freqs == NULL)
                || pred_DC == NULL)
                return;


        diff = bloc[n] - *pred_DC;
        *pred_DC = bloc[n];
        n++;


        class = magnitude_class(diff);

        if (freqs)
                freqs[0][class]++;

        else {
                write_huffman_value(class, table_DC, stream);
                write_magnitude(stream, diff);
        }


        while (n < BLOCK_SIZE) {

                uint8_t i = n;
                zeros = 0;

                while (i < BLOCK_SIZE && !bloc[i++])
                        zeros++;


                /* Only zeros left */
                if (zeros >= (BLOCK_SIZE - n)) {
                        symbol = EOB;
                        n = BLOCK_SIZE;


                        if (freqs)
                                freqs[1][symbol]++;

                        else
                                write_huffman_value(symbol, table_AC, stream);

                /* At least 16 zeros */
                } else if (zeros >= 16) {
                        symbol = ZRL;
                        n += 16;


                        if (freqs)
                                freqs[1][symbol]++;

                        else
                                write_huffman_value(symbol, table_AC, stream);

                } else {
                        n += zeros;

                        class = magnitude_class(bloc[n]);
                        symbol = (zeros << 4) | (class & 0xF);


                        if (freqs)
                                freqs[1][symbol]++;

                        else {
                                write_huffman_value(symbol, table_AC, stream);
                                write_magnitude(stream, bloc[n]);
                        }

                        n++;
                }
        }
}

