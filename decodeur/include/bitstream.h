#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__

#include <stdint.h>
#include <stdbool.h>


struct bitstream;


/* Open the filename file as bitstream */
extern struct bitstream *create_bitstream(const char *filename);

/* Returns true if eof is reached */
extern bool end_of_bitstream(struct bitstream *stream);

/*
 * Read nb_bits from the stream and return those bits in dest.
 * byte_stuffing indicates if we have to read using byte stuffing.
 */
extern uint8_t read_bitstream(struct bitstream *stream, uint8_t nb_bits,
                                uint32_t *dest, bool byte_stuffing);

/* Read in the stream until the value "byte" is found or the end of file */
extern bool skip_bitstream_until(struct bitstream *stream, uint8_t byte);

/* Close the stream and free all memory */
extern void free_bitstream(struct bitstream *stream);


#endif

