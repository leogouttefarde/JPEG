/* Projet C - Sujet JPEG */
#ifndef __HUFFMAN_H__
#define __HUFFMAN_H__

#include <stdint.h>
#include <stdbool.h>
#include "bitstream.h"


/*
 * Huffman table node structure
 */
struct huff_table;


/*
 * Loads a Huffman table from the input stream.
 */
extern struct huff_table *load_huffman_table(
                struct bitstream *stream, uint16_t *nb_byte_read);

/*
 * Reads a Huffman value from the input stream.
 */
extern int8_t next_huffman_value(struct huff_table *table, 
                struct bitstream *stream);

/*
 * Recursively frees a Huffman table.
 */
extern void free_huffman_table(struct huff_table *table);

/*
 * Writes a value using its Huffman code.
 */
extern bool write_huffman_value(int8_t value, struct huff_table *table,
                         struct bitstream *stream,
                         uint32_t **freqs, uint8_t freq_type);

/*
 * Creates a Huffman tree according
 * to given input frequencies.
 */
extern struct huff_table *create_huffman_tree(uint32_t freqs[0x100], bool *error);

/*
 * Writes a Huffman table into the stream.
 */
extern void write_huffman_table(struct bitstream *stream, struct huff_table **table);

/*
 * Exports the input Huffman table to
 * a dot tree file (for debugging).
 */
extern void huffman_export(char *dest, struct huff_table *table);


#endif

