/* Projet C - Sujet JPEG */
#ifndef __HUFFMAN_H__
#define __HUFFMAN_H__

#include <stdint.h>
#include <stdbool.h>
#include "bitstream.h"


struct huff_table;

extern struct huff_table *load_huffman_table(
                struct bitstream *stream, uint16_t *nb_byte_read);

extern int8_t next_huffman_value(struct huff_table *table, 
                struct bitstream *stream);

extern void free_huffman_table(struct huff_table *table);

extern bool write_huffman_value(int8_t value, struct huff_table *table,
                         struct bitstream *stream,
                         uint32_t **freqs, uint8_t freq_type);

extern struct huff_table *create_huffman_tree(uint32_t freqs[0x100], bool *error);

void write_huffman_table(struct bitstream *stream, struct huff_table **table);

void huffman_export(char *dest, struct huff_table *table);


#endif

