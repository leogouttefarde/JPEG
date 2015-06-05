
#ifndef __LIBRARY_H__
#define __LIBRARY_H__

#include "bitstream.h"
#include "common.h"
#include "jpeg.h"


bool read_short_BE(struct bitstream *stream, uint16_t *value);
bool read_byte(struct bitstream *stream, uint8_t *value);
bool skip_bitstream(struct bitstream *stream, uint32_t nb_bytes);


uint32_t truncate(int32_t s);

uint8_t double2uint8(double x);


bool is_valid_jpeg(char *path);
bool is_valid_tiff(char *path);

bool parse_args(int argc, char **argv, struct options *options);


#endif
