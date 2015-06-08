#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__

#include <stdint.h>
#include <stdbool.h>


struct bitstream;

/* Initialize the bitstream structure with the file filename */
extern struct bitstream *create_bitstream(const char *filename);

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


#endif

