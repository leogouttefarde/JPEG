/* Projet C - Sujet JPEG */
#ifndef __PACK_H__
#define __PACK_H__

#include "unpack.h"


extern void pack_block(struct bitstream *stream,
                struct huff_table *table_DC, int32_t *pred_DC,
                struct huff_table *table_AC,
                int32_t bloc[64], uint32_t **freqs);

#endif

