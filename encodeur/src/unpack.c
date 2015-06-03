
#include "unpack.h"
#include "common.h"


enum ac_symbols {
        ZRL = 0xF0,
        EOB = 0x00
};

static inline int16_t extract_magnitude(struct bitstream *stream, uint8_t class)
{
        bool negative = false;
        int8_t bit;
        int16_t value = 0;
        uint32_t dest;
        // printf("extract_magnitude :  ");

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
                //printf("\n");

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
                // Error
                return;


        class = next_huffman_value(table_DC, stream);
        diff = extract_magnitude(stream, class);
        // printf("read DC val : %02X\n", class);

        bloc[n] = *pred_DC + diff;
        *pred_DC = bloc[n];
        n++;


        while (n < BLOCK_SIZE) {
                huffman_value = next_huffman_value(table_AC, stream);
                // printf("read AC val : %02X\n", huffman_value);

                switch ((uint8_t)huffman_value) {
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
                        zeros = ((uint8_t)huffman_value) >> 4;

                        for (uint8_t i = 0; i < zeros; i++)
                                bloc[n + i] = 0;

                        n += zeros;

                        bloc[n++] = extract_magnitude(stream, class);
                }
        }
}

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
        //printf("writ magnitude, c:%d, v= %02X\n", class, (uint16_t)value);

        if (class > 0) {
                if (value < 0)
                        value += (1 << class) - 1;

                for (uint8_t k = 0; k < class; ++k) {
                        bit = (value >> (class - 1 - k)) & 1;
                        write_bit(stream, bit, true);
                        // printf("%d", bit);
                }
                //printf("\n");
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
        //printf("pack_block\n");

        if (table_AC == NULL || table_DC == NULL || pred_DC == NULL)
                // Error
                return;


        diff = bloc[n] - *pred_DC;
        *pred_DC = bloc[n];
        n++;


        class = magnitude_class(diff);

        //printf("writ DC val : %02X\n", class);
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


                // Si plus que des zéros
                if (zeros >= (BLOCK_SIZE - n)) {
                        symbol = EOB;
                        n = BLOCK_SIZE;


                        //printf("writ AC val : %02X\n", symbol);

                        if (freqs)
                                freqs[1][symbol]++;

                        else
                                write_huffman_value(symbol, table_AC, stream);

                // Si au moins 16 zéros
                } else if (zeros >= 16) {
                        symbol = ZRL;
                        n += 16;

                        //printf("writ AC val : %02X\n", symbol);
                        if (freqs)
                                freqs[1][symbol]++;

                        else
                                write_huffman_value(symbol, table_AC, stream);

                } else {
                        n += zeros;

                        class = magnitude_class(bloc[n]);
                        symbol = (zeros << 4) | (class & 0xF);

                        // printf("zeros = %02X\n", zeros);
                        //printf("class = %02X\n", class);
                        //printf("writ AC val : %02X\n", symbol);
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

