#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


/*
 * Stream opening modes
 */
enum stream_mode {

        /* Read only */
        RDONLY = 1,

        /* Write only */
        WRONLY = 2
};

/*
 * Internal bitstream structure
 */
struct bitstream;


/*
 * Opens the filename file as bitstream
 * with the right mode (read / write)
 */
extern struct bitstream *create_bitstream(const char *filename, 
                                          enum stream_mode mode);

/* Returns true if eof is reached */
extern bool end_of_bitstream(struct bitstream *stream);

/*
 * Read nb_bits from the stream and return those bits in dest.
 * byte_stuffing indicates if we have to read using byte stuffing.
 */
extern uint8_t read_bitstream(struct bitstream *stream, 
                uint8_t nb_bits, uint32_t *dest,
                bool byte_stuffing);

/* Read in the stream until the value "byte" is found or the end of file */
extern bool skip_bitstream_until(struct bitstream *stream, uint8_t byte);

/* Close the stream and free all memory */
extern void free_bitstream(struct bitstream *stream);

/* Write a bit into the stream */
extern int8_t write_bit(struct bitstream *stream, uint8_t bit, 
                        bool byte_stuffing);

/* Write a byte into the stream */
extern void write_byte(struct bitstream *stream, uint8_t byte);

/* Write a short as big endian into the stream*/
extern void write_short_BE(struct bitstream *stream, uint16_t val);

/* Seek the stream to a specific position */
extern void seek_bitstream(struct bitstream *stream, uint32_t pos);

/* Returns the current stream position */
extern uint32_t pos_bitstream(struct bitstream *stream);

/*
 * Writes all remaining bits to the stream if necessary
 */
extern void flush_bitstream(struct bitstream *stream);


#endif
