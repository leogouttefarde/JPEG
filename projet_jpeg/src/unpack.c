
#include "unpack.h"
#include "common.h"


enum RLE {
        ZRL = 0xF0,
        EOB = 0x00
};


static inline int16_t read_magnitude(struct bitstream *stream, uint8_t class)
{
        bool negative = false;
        int8_t bit;
        int16_t value = 0;
        uint32_t dest;

        if (class > 0) {
                read_bitstream(stream, 1, &dest, true);
                bit = dest & 1;
                value = bit;

                if (value == 0)
                        negative = true;

                for (uint8_t k = 1; k < class; ++k) {
                        read_bitstream(stream, 1, &dest, true);
                        bit = (dest & 1);

                        value = (value << 1) | bit;
                }

                if (negative)
                        value = -1 * ((1 << class) - 1 - value);
        }

        return value;
}

void unpack_block(struct bitstream *stream,
                struct huff_table *table_DC, int32_t *pred_DC,
                struct huff_table *table_AC,
                int32_t bloc[64])
{
        uint8_t class, zeros, huffman_value;
        uint8_t n = 0;
        int16_t diff;

        if (table_AC == NULL || table_DC == NULL || pred_DC == NULL)
                return;

	/* Retrieve the DC coefficient */
        class = next_huffman_value(table_DC, stream);
        diff = read_magnitude(stream, class);

        bloc[n] = *pred_DC + diff;
        *pred_DC = bloc[n];
        n++;

	/* Retrieve the 63 AC coefficients */
        while (n < BLOCK_SIZE) {
                huffman_value = next_huffman_value(table_AC, stream);

                switch (huffman_value) {
                case ZRL:
                        for (uint8_t i = 0; i < 16; i++)
                                bloc[n + i] = 0;

                        n += 16;
                        break;

                case EOB:
                        for (; n < BLOCK_SIZE; n++)
                                bloc[n] = 0;
                        break;

                default:
                        class = huffman_value & 0xF;
                        zeros = huffman_value >> 4;

                        for (uint8_t i = 0; i < zeros; i++)
                                bloc[n + i] = 0;

                        n += zeros;

                        bloc[n++] = read_magnitude(stream, class);
                }
        }
}

