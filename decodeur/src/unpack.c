
#include "unpack.h"
#include "common.h"


/*
 * RLE compression codes
 */
enum RLE {
        ZRL = 0xF0,
        EOB = 0x00
};

/*
 * Reads a magnitude from the specified class
 */
static int16_t read_magnitude(struct bitstream *stream, uint8_t class)
{
        bool negative = false;
        int8_t bit;
        int16_t value = 0;
        uint32_t dest;

        /* Magnitude 0 is never used */
        if (class > 0) {

                /* Read the first bit */
                read_bitstream(stream, 1, &dest, true);
                bit = dest & 1;
                value = bit;

                /*
                 * Negative magnitude values always start with a 0
                 * Positive magnitude values always start with a 1
                 */
                if (value == 0)
                        negative = true;

                /* Other bits represent the positive magnitude value */
                for (uint8_t k = 1; k < class; ++k) {
                        read_bitstream(stream, 1, &dest, true);
                        bit = (dest & 1);

                        value = (value << 1) | bit;
                }

                /* Compute the corresponding negative value*/
                if (negative)
                        value = -1 * ((1 << class) - 1 - value);
        }

        return value;
}

/*
 * Reads and unpacks an 8x8 JPEG data block from stream
 */
void unpack_block(struct bitstream *stream,
                struct huff_table *table_DC, int32_t *pred_DC,
                struct huff_table *table_AC, int32_t bloc[64])
{
        uint8_t class, zeros, huffman_value;
        uint8_t n = 0;
        int16_t diff;

        /* Error handling */
        if (table_AC == NULL || table_DC == NULL || pred_DC == NULL)
                return;


        /* Read the DC magnitude class */
        class = next_huffman_value(table_DC, stream);

        /* Read the DC magnitude difference value */
        diff = read_magnitude(stream, class);

        /* Compute the DC value according to the last one */
        bloc[n] = *pred_DC + diff;

        /* Update the last value field */
        *pred_DC = bloc[n++];


        /* Retrieve the 63 AC coefficients */
        while (n < BLOCK_SIZE) {

                /* Read the next AC symbol */
                huffman_value = next_huffman_value(table_AC, stream);


                /* Process it accordingly */
                switch (huffman_value) {

                /* Next 16 AC coefficients are 0 */
                case ZRL:
                        for (uint8_t i = 0; i < 16; i++)
                                bloc[n + i] = 0;

                        n += 16;
                        break;

                /* All remaining AC coefficients are 0 */
                case EOB:
                        for (; n < BLOCK_SIZE; n++)
                                bloc[n] = 0;
                        break;

                /* Regular processing */
                default:
                        class = huffman_value & 0xF;
                        zeros = huffman_value >> 4;

                        /* Set 0 AC values */
                        for (uint8_t i = 0; i < zeros; i++)
                                bloc[n + i] = 0;

                        n += zeros;

                        /* Read the next non-zero AC value as magnitude */
                        bloc[n++] = read_magnitude(stream, class);
                }
        }
}

