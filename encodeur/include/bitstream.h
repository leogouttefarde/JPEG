#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


enum stream_mode {
        RDONLY = 1,
        WRONLY = 2
};


struct bitstream;

extern struct bitstream *create_bitstream(const char *filename, enum stream_mode mode);
// extern struct bitstream *create_bitstream(const char *filename);

extern bool end_of_bitstream(struct bitstream *stream);

extern uint8_t read_bitstream(struct bitstream *stream, 
                uint8_t nb_bits, uint32_t *dest,
                bool byte_stuffing);

extern bool skip_bitstream_until(struct bitstream *stream, uint8_t byte);

extern void free_bitstream(struct bitstream *stream);


int8_t write_bit(struct bitstream *stream, uint8_t bit, bool byte_stuffing);

void write_byte(struct bitstream *stream, uint8_t byte);

void write_short_BE(struct bitstream *stream, uint16_t val);

void seek_bitstream(struct bitstream *stream, uint32_t pos);

uint32_t pos_bitstream(struct bitstream *stream);

void flush_bitstream(struct bitstream *stream);



#endif

