#ifndef __CONV_H__
#define __CONV_H__

#include <stdint.h>

extern void YCbCr_to_ARGB(uint8_t  *mcu_YCbCr[3], uint32_t *mcu_RGB,
		uint32_t nb_blocks_h, uint32_t nb_blocks_v);

void Y_to_ARGB(uint8_t *mcu_Y, uint32_t *mcu_RGB,
                uint32_t nb_blocks_h, uint32_t nb_blocks_v);


#endif

