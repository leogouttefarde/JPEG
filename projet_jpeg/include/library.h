
#ifndef __LIBRARY_H__
#define __LIBRARY_H__

#include "bitstream.h"
#include "common.h"
#include "jpeg.h"

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
 * Checks that the filename extension is "jpg" or "jpeg"
 */
extern bool is_valid_ext(char *path);

/*
 * Skips nb_bytes bytes in the stream
 */
extern bool skip_bitstream(struct bitstream *stream, uint32_t nb_bytes);

/*
 * Parses arguments given to the program and puts them in *options
 */
extern bool parse_args(int argc, char **argv, struct options *options);

/*
 * Prints a BLOCK_DIMxBLOCK_DIM block of int32_t for debug puposes.
 */
extern bool print_block(int32_t *block);

#endif
