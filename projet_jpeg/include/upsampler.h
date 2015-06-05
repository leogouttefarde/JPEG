/* Projet C - Sujet JPEG */
#ifndef UPSAMPLER_H__
#define UPSAMPLER_H__

#include <stdint.h>


extern void upsampler(uint8_t *in,
		uint8_t nb_blocks_in_h, uint8_t nb_blocks_in_v,
		uint8_t *out,
		uint8_t nb_blocks_out_h, uint8_t nb_blocks_out_v);


extern void downsampler(uint8_t *in,
			uint8_t nb_blocks_in_h, uint8_t nb_blocks_in_v,
			uint8_t *out,
			uint8_t nb_blocks_out_h, uint8_t nb_blocks_out_v);

#endif

