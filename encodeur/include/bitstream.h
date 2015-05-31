#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__

#include <stdint.h>
#include <stdbool.h>


struct bitstream;

extern struct bitstream *create_bitstream(const char *filename);

extern bool end_of_bitstream(struct bitstream *stream);

extern uint8_t read_bitstream(struct bitstream *stream, 
                uint8_t nb_bits, uint32_t *dest,
                bool byte_stuffing);

extern bool skip_bitstream_until(struct bitstream *stream, uint8_t byte);

extern void free_bitstream(struct bitstream *stream);


#endif

