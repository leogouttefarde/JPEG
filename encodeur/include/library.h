
#ifndef __LIBRARY_H__
#define __LIBRARY_H__

#include "bitstream.h"
#include "common.h"
#include "encode.h"

#define TRUNCATE(x) ((uint8_t)((x > 255) ? 255 : ( (x < 0) ? 0 : (uint32_t)x )))


bool read_short_BE(struct bitstream *stream, uint16_t *value);
bool read_byte(struct bitstream *stream, uint8_t *value);
bool skip_bitstream(struct bitstream *stream, uint32_t nb_bytes);


uint32_t truncate(int32_t s);

uint8_t double2uint8(double x);


bool is_valid_jpeg(char *path);
bool is_valid_tiff(char *path);

bool parse_args(int argc, char **argv, struct options *options);


uint32_t *mcu_to_image(
        uint32_t *data, struct mcu_info *mcu,
        uint32_t width, uint32_t height);

uint32_t *image_to_mcu(
        uint32_t *image, struct mcu_info *mcu,
        uint32_t width, uint32_t height);


#endif
