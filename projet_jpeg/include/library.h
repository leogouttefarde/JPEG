
#ifndef __LIBRARY_H__
#define __LIBRARY_H__

#include "bitstream.h"
#include "common.h"
#include "jpeg.h"

#define TRUNCATE(x) ((uint8_t)((x > 255) ? 255 : ( (x < 0) ? 0 : (uint32_t)x )))


bool read_short_BE(struct bitstream *stream, uint16_t *value);
bool read_byte(struct bitstream *stream, uint8_t *value);
bool is_valid_ext(char *path);
bool skip_bitstream(struct bitstream *stream, uint32_t nb_bytes);

bool parse_args(int argc, char **argv, struct options *options);

#endif
