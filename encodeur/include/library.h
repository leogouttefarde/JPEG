
#ifndef __LIBRARY_H__
#define __LIBRARY_H__

#include "bitstream.h"
#include "common.h"
#include "encode.h"

/*
 * Forces x value in [0,255]
 */
#define TRUNCATE(x) ((uint8_t)((x > 255) ? 255 : ( (x < 0) ? 0 : (uint32_t)x )))

/*
 * Reads a short from stream encoded in Big Endian
 */
extern bool read_short_BE(struct bitstream *stream, uint16_t *value);

/*
 * Reads a byte from stream
 */
extern bool read_byte(struct bitstream *stream, uint8_t *value);

/*
 * Skips nb_bytes bytes in the stream
 */
bool skip_bitstream(struct bitstream *stream, uint32_t nb_bytes);


uint32_t truncate(int32_t s);

uint8_t double2uint8(double x);

/*
 * Verifies that the filename extension is "jpg" or "jpeg"
 */
extern bool is_valid_jpeg(char *path);

/*
 * Verifies that the filename extension is "tiff" or "tif"
 */
extern bool is_valid_tiff(char *path);

/*
 * Parses arguments given to the program and puts them in *options
 */
extern bool parse_args(int argc, char **argv, struct options *options);

/*
 * Converts MCU to lines representation
 */
extern uint32_t *mcu_to_image(
        uint32_t *data, struct mcu_info *mcu,
        uint32_t width, uint32_t height);

/*
 * Converts lines to MCU representation
 */
extern uint32_t *image_to_mcu(
        uint32_t *image, struct mcu_info *mcu,
        uint32_t width, uint32_t height);

/*
 * Process options
 */
extern void process_options(struct options *options, struct jpeg_data *jpeg, bool *error);

/*
 * Exports the jpeg to the tiff file
 */
extern void export_tiff(struct jpeg_data *jpeg, bool *error);

/*
 * Converts all pixels to grayscale
 */
extern void compute_gray(struct jpeg_data *jpeg);

/*
 * Prints a block of BLOCK_DIMxBLOCK_DIM of int32_t for debug puposes.
 */
extern bool print_block(int32_t *block);

/*
 * Prints a block of BLOCK_DIMxBLOCK_DIM of int8_t for debug puposes.
 */
extern bool print_byte_block(uint8_t *block);

#endif
