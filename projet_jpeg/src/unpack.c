
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
        printf("read_magnitude :  ");

        if (class > 0) {
                read_bitstream(stream, 1, &dest, true);
                bit = dest & 1;
                value = bit;
                // printf("%d", bit);

                if (value == 0)
                        negative = true;

                for (uint8_t k = 1; k < class; ++k) {
                        read_bitstream(stream, 1, &dest, true);
                        bit = (dest & 1);
                        // printf("%d", bit);

                        value = (value << 1) | bit;
                }

                if (negative)
                        value = -1 * ((1 << class) - 1 - value);
        }
        printf("\n");
        // printf("\nread magnitude, c:%d, v= %02X\n", class, (uint16_t)value);

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
                // Error
                return;


        class = next_huffman_value(table_DC, stream);
        printf("read DC val : %02X\n", class);
        diff = read_magnitude(stream, class);

        bloc[n] = *pred_DC + diff;
        *pred_DC = bloc[n];
        n++;


        while (n < BLOCK_SIZE) {
                huffman_value = next_huffman_value(table_AC, stream);
                printf("read AC val : %02X\n", huffman_value);

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
                        printf("zeros = %02X\n", zeros);
                        printf("class = %02X\n", class);

                        for (uint8_t i = 0; i < zeros; i++)
                                bloc[n + i] = 0;

                        n += zeros;

                        bloc[n++] = read_magnitude(stream, class);
                }
        }
}

