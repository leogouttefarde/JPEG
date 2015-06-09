/* Projet C - Sujet JPEG */
#ifndef __HUFFMAN_H__
#define __HUFFMAN_H__

#include <stdint.h>
#include <stdbool.h>
#include "bitstream.h"


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
 * Recursively free a Huffman table.
 */
extern void free_huffman_table(struct huff_table *table);


#endif

