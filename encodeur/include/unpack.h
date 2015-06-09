/* Projet C - Sujet JPEG */
#ifndef __UNPACK_H__
#define __UNPACK_H__

#include <stdint.h>
#include "bitstream.h"
#include "huffman.h"


enum RLE {
        ZRL = 0xF0,
        EOB = 0x00
};


extern void unpack_block(struct bitstream *stream,
		struct huff_table *table_DC, int32_t *pred_DC,
		struct huff_table *table_AC,
		int32_t bloc[64]);

#endif

