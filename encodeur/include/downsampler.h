/* Projet C - Sujet JPEG */
#ifndef __DOWNSAMPLER_H__
#define __DOWNSAMPLER_H__

#include <stdint.h>


/* Downsamples any input component's blocks */
extern void downsampler(uint8_t *in,
                uint8_t nb_blocks_out_h, uint8_t nb_blocks_out_v,
		uint8_t *out,
                uint8_t nb_blocks_in_h, uint8_t nb_blocks_in_v);


#endif

