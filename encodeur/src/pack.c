
#include "pack.h"
#include "common.h"


/*
 * Detects the required magnitude
 * to encode a value
 */
static inline uint8_t magnitude_class(int16_t value)
{
        uint8_t class = 0;

        /*
         * Compute the positive value
         * to encode (simplifies detection)
         */
        if (value < 0)
                value *= -1;

        /*
         * The required magnitude corresponds
         * to the number of bits to encode
         */
        while (value) {
                value >>= 1;

                class++;
        }

        return class;
}

/*
 * Writes a value as magnitude
 */
static uint8_t write_magnitude(struct bitstream *stream, int16_t value)
{
        uint8_t bit;

        /* Compute the value's magnitude class */
        uint8_t class = magnitude_class(value);

        /* Magnitude 0 is never used */
        if (class > 0) {

                /*
                 * Convert negative values to their
                 * corresponding magnitude value
                 */
                if (value < 0)
                        value += (1 << class) - 1;

                /*
                 * Write the value as big endian bits
                 */
                for (uint8_t k = 0; k < class; ++k) {
                        bit = (value >> (class - 1 - k)) & 1;
                        write_bit(stream, bit, true);
                }
        }

        return class;
}

/*
 * Packs and writes an 8x8 JPEG data block to stream.
 * If freqs is not NULL, computes written values's frequencies
 * to build Huffman trees.
 */
void pack_block(struct bitstream *stream,
                struct huff_table *table_DC, int32_t *pred_DC,
                struct huff_table *table_AC,
                int32_t bloc[64], uint32_t **freqs)
{
        uint8_t class, zeros, symbol, i;
        uint8_t n = 0;
        int16_t diff;

        /* Error handling */
        if ( ((table_AC == NULL || table_DC == NULL) && freqs == NULL)
                || pred_DC == NULL)
                return;


        /*
         * Compute the DC difference according
         * to the last value and update the last value field
         */
        diff = bloc[n] - *pred_DC;
        *pred_DC = bloc[n];
        n++;

        /* Compute the required magnitude class */
        class = magnitude_class(diff);

        /* Write the magnitude class as Huffman value */
        write_huffman_value(class, table_DC, stream, freqs, 0);

        /* Write the DC difference as magnitude value */
        if (freqs == NULL)
                write_magnitude(stream, diff);


        /* Write the 63 AC coefficients */
        while (n < BLOCK_SIZE) {

                i = n;
                zeros = 0;

                /* Count next zero values */
                while (i < BLOCK_SIZE && !bloc[i++])
                        zeros++;


                /* Only zeros left */
                if (zeros >= (BLOCK_SIZE - n)) {
                        symbol = EOB;
                        n = BLOCK_SIZE;

                        write_huffman_value(symbol, table_AC, stream, freqs, 1);

                /* At least 16 zeros */
                } else if (zeros >= 16) {
                        symbol = ZRL;
                        n += 16;

                        write_huffman_value(symbol, table_AC, stream, freqs, 1);

                /* 15 zeros or less */
                } else {
                        n += zeros;

                        class = magnitude_class(bloc[n]);
                        symbol = (zeros << 4) | (class & 0xF);

                        write_huffman_value(symbol, table_AC, stream, freqs, 1);

                        if (freqs == NULL)
                                write_magnitude(stream, bloc[n]);

                        n++;
                }
        }
}

