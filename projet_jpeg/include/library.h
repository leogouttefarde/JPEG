
#ifndef __LIBRARY_H__
#define __LIBRARY_H__

#include "bitstream.h"
#include "common.h"
#include "jpeg.h"


bool read_short_BE(struct bitstream *stream, uint16_t *value);
bool read_byte(struct bitstream *stream, uint8_t *value);
bool is_valid_ext(char *path);
bool skip_bitstream(struct bitstream *stream, uint32_t nb_bytes);

uint8_t truncate(double x);

bool parse_args(int argc, char **argv, struct options *options);

#endif
