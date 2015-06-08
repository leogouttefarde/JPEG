#ifndef __CONV_H__
#define __CONV_H__

#include <stdint.h>

/* Convert YCbCr MCU to RGB MCU */
extern void YCbCr_to_ARGB(uint8_t  *mcu_YCbCr[3], uint32_t *mcu_RGB,
		uint32_t nb_blocks_h, uint32_t nb_blocks_v);

/* Convert RGB MCU to YCbCr MCU */
void ARGB_to_YCbCr(uint32_t *mcu_RGB, uint8_t  *mcu_YCbCr[3],
                uint32_t nb_blocks_h, uint32_t nb_blocks_v);

/* 
 * Convert Y MCU to RGB MCU
 * Use for Grayscale image
 */
void Y_to_ARGB(uint8_t *mcu_Y, uint32_t *mcu_RGB,
                uint32_t nb_blocks_h, uint32_t nb_blocks_v);

/* 
 * Convert RGB MCU to Y MCU
 * Use for Grayscale image
 */
void ARGB_to_Y(uint32_t *mcu_RGB, uint8_t  *mcu_Y,
                uint32_t nb_blocks_h, uint32_t nb_blocks_v);


#endif

