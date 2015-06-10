
#ifndef __LIBRARY_H__
#define __LIBRARY_H__

#include "bitstream.h"
#include "common.h"
#include "encode.h"

/*
 * Truncation macro
 * Enforces input value into [0,255]
 */
#define TRUNCATE(x) ((uint8_t)((x > 255) ? 255 : ( (x < 0) ? 0 : (uint32_t)x )))


/*
 * Reads a short from stream as Big Endian
 */
extern bool read_short_BE(struct bitstream *stream, uint16_t *value);

/*
 * Reads a byte from stream
 */
extern bool read_byte(struct bitstream *stream, uint8_t *value);

/*
 * Skips nb_bytes bytes in the stream
 */
extern bool skip_bitstream(struct bitstream *stream, uint32_t nb_bytes);

/*
 * Checks that the filename extension is "jpg" or "jpeg"
 */
extern bool is_valid_jpeg(char *path);

/*
 * Checks that the filename extension is "tiff" or "tif"
 */
extern bool is_valid_tiff(char *path);

/*
 * Parses arguments given to the program and puts them in *options
 */
extern bool parse_args(int argc, char **argv, struct options *options);

/*
 * Converts an MCU image to a regular image.
 */
extern uint32_t *mcu_to_image(
        uint32_t *data, struct mcu_info *mcu,
        uint32_t width, uint32_t height);

/*
 * Converts an image to an MCU image.
 */
extern uint32_t *image_to_mcu(
        uint32_t *image, struct mcu_info *mcu,
        uint32_t width, uint32_t height);

/*
 * Process specific options.
 */
extern void process_options(struct options *options, struct jpeg_data *jpeg, bool *error);

/*
 * Exports raw MCU image data as TIFF.
 */
extern void export_tiff(struct jpeg_data *jpeg, bool *error);

/*
 * Enforces all pixels to grayscale.
 */
extern void compute_gray(struct jpeg_data *jpeg);

/*
 * Prints a BLOCK_DIMxBLOCK_DIM block of int32_t for debug purposes.
 */
extern bool print_block(int32_t *block);

/*
 * Prints a BLOCK_DIMxBLOCK_DIM block of uint8_t for debug purposes.
 */
extern bool print_byte_block(uint8_t *block);


#endif
