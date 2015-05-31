
#ifndef __LIBRARY_H__
#define __LIBRARY_H__

#include "bitstream.h"
#include "common.h"


bool read_short_BE(struct bitstream *stream, uint16_t *value);
bool read_byte(struct bitstream *stream, uint8_t *value);
bool is_valid_ext(char *path);
bool skip_bitstream(struct bitstream *stream, uint32_t nb_bytes);


inline uint32_t truncate(int32_t s)
{
        s = (s > 255) ? 255 : ( (s < 0) ? 0 : s );

        return (uint32_t)s;
}

inline uint8_t double2uint8(double x)
{
        uint8_t res;

        res = (x > 255) ? 255 : ( (x < 0) ? 0 : (uint8_t)x );

        return res;
}


#endif
