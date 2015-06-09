#ifndef __CONV_H__
#define __CONV_H__

#include <stdint.h>


/* Convert YCbCr MCUs to RGB MCUs */
extern void YCbCr_to_ARGB(uint8_t  *mcu_YCbCr[3], uint32_t *mcu_RGB,
                          uint32_t nb_blocks_h, uint32_t nb_blocks_v);

/*
 * Convert Y MCUs to RGB MCUs
 * Used for grayscale images
 */
extern void Y_to_ARGB(uint8_t *mcu_Y, uint32_t *mcu_RGB,
                      uint32_t nb_blocks_h, uint32_t nb_blocks_v);

/* Convert RGB MCUs to YCbCr MCUs */
extern void ARGB_to_YCbCr(uint32_t *mcu_RGB, uint8_t  *mcu_YCbCr[3],
                          uint32_t nb_blocks_h, uint32_t nb_blocks_v);

/*
 * Convert RGB MCUs to Y MCUs
 * Used for grayscale images
 */
extern void ARGB_to_Y(uint32_t *mcu_RGB, uint8_t  *mcu_Y,
                      uint32_t nb_blocks_h, uint32_t nb_blocks_v);


#endif

