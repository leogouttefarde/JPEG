
#include "unpack.h"
#include "common.h"


int16_t extract_dpcm(struct bitstream *stream, uint8_t nb_bits)
{
        bool negative = false;
        int8_t bit;
        int16_t value = 0;
        uint32_t dest;

        if (nb_bits > 0) {
                read_bitstream(stream, 1, &dest, true);
                bit = dest & 1;
                value = bit;

                if (value == 0)
                        negative = true;

                for (uint8_t k = 1; k < nb_bits; ++k) {
                        read_bitstream(stream, 1, &dest, true);
                        bit = (dest & 1);

                        value = (value << 1) | bit;
                }

                if (negative)
                        value = -1 * ((1 << nb_bits) - 1 - value);
        }

        return value;
}

void unpack_block(struct bitstream *stream,
                struct huff_table *table_DC, int32_t *pred_DC,
                struct huff_table *table_AC,
                int32_t bloc[64])
{
        int8_t nb_bits, zeros, symbol;
        uint8_t n = 0;
        int16_t diff;

        if (table_AC == NULL || table_DC == NULL || pred_DC == NULL)
                // Error
                return;


        nb_bits = next_huffman_value(table_DC, stream);
        diff = extract_dpcm(stream, nb_bits);

        bloc[n] = *pred_DC + diff;
        *pred_DC = bloc[n];
        n++;


        while (n < BLOCK_SIZE) {
                symbol = next_huffman_value(table_AC, stream);

                switch ((uint8_t)symbol) {
                case 0xF0:
                        for (uint8_t i = 0; i < 16; i++)
                                bloc[n + i] = 0;

                        n += 16;
                        break;

                case 0x00:
                        for (; n < BLOCK_SIZE; n++)
                                bloc[n] = 0;
                        break;

                default:
                        nb_bits = symbol & 0xF;
                        zeros = ((uint8_t)symbol) >> 4;

                        for (uint8_t i = 0; i < zeros; i++)
                                bloc[n + i] = 0;

                        n += zeros;

                        bloc[n++] = extract_dpcm(stream, nb_bits);
                }
        }
}

