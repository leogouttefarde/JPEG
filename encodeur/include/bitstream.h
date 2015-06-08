#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


enum stream_mode {
        RDONLY = 1, /* Read */
        WRONLY = 2 /* Write */
};


struct bitstream;

/* Initialize the bitstream structure with the file filename */
extern struct bitstream *create_bitstream(const char *filename, 
					  enum stream_mode mode);

/* Return true if eof is reach */
extern bool end_of_bitstream(struct bitstream *stream);

/* 
 * Read nb_bits in the stream and return those bits in dest 
 * byte_stuffing indicate if we have to take account of the byte_stuffing
 */
extern uint8_t read_bitstream(struct bitstream *stream, 
                uint8_t nb_bits, uint32_t *dest,
                bool byte_stuffing);

/* Read in the stream until the value "byte" is found or the end of file */
extern bool skip_bitstream_until(struct bitstream *stream, uint8_t byte);

/* Close the stream and free the memory of the stream */
extern void free_bitstream(struct bitstream *stream);

/* 
 * Save the bit "bit" in the stream buffer byte
 * And write the buffer if it contain 8 saved bits
 */
extern int8_t write_bit(struct bitstream *stream, uint8_t bit, 
			bool byte_stuffing);

/* Write the byte "byte" in the stream */
extern void write_byte(struct bitstream *stream, uint8_t byte);

/* Write the short val in big endian in the stream */
extern void write_short_BE(struct bitstream *stream, uint16_t val);

/* Seek the position pos from the begining of the stream file */
extern void seek_bitstream(struct bitstream *stream, uint32_t pos);

/* Return the current position of the stream file*/
extern uint32_t pos_bitstream(struct bitstream *stream);

/* 
 * Write the content of the buffer byte in the stream 
 * by adding 0 if byte doesn't contain 8 bits
 * Use to flush byte
 */
extern void flush_bitstream(struct bitstream *stream);



#endif

