
#include "bitstream.h"
#include "common.h"


/*
 * Internal bitstream structure
 */
struct bitstream {

        /* Currently opened file */
        FILE *file;

        /* Opened file's mode */
        enum stream_mode mode;

        /* Current byte */
        uint8_t byte;

        /* Next bit's index in byte */
        uint8_t index;
};


/*
 * Opens the filename file as bitstream
 * with the right mode (read / write)
 */
struct bitstream *create_bitstream(const char *filename, enum stream_mode mode)
{
        struct bitstream *stream = NULL;

        if (filename != NULL) {
                char *open_mode;

                if (mode == RDONLY)
                        open_mode = "rb";

                else if (mode == WRONLY)
                        open_mode = "wb";

                else
                        open_mode = "r+";

                /* Open the filename file in open_mode (read, write or both) */
                FILE *file = fopen(filename, open_mode);

                /* Create and initialize the stream */
                if (file != NULL) {
                        stream = malloc(sizeof(struct bitstream));

                        if (stream != NULL) {
                                stream->file = file;
                                stream->mode = mode;
                                stream->byte = 0;
                                stream->index = 0;
                        }
                        else
                                fclose(file);
                }
        }

        return stream;
}

/* Returns true if eof is reached */
bool end_of_bitstream(struct bitstream *stream)
{
        bool end = true;

        if (stream != NULL && stream->file != NULL) {
                end = feof(stream->file) ? true : false;
        }

        return end;
}

/* Read the next bit from the stream */
int8_t next_bit(struct bitstream *stream, bool byte_stuffing)
{
        uint32_t size = 0;
        int8_t bit;
        uint8_t *byte = &stream->byte;
        
        /* Reads a new byte */
        if (stream->index == 0) {

                uint8_t last = *byte;
                size = fread(byte, 1, 1, stream->file);
                
                /* Byte_stuffing */
                if (byte_stuffing && last == 0xFF) {
                        if (*byte != 0x00)
                                return -1;

                        size = fread(byte, 1, 1, stream->file);
                }
                
                /* Error handling */
                if (size == 0)
                        return -2;

                stream->index = 8;
        }
        
        /* Compute the next bit */
        bit = 1 & (*byte >> --stream->index);

        return bit;
}

/*
 * Read nb_bits from the stream and return those bits in dest.
 * byte_stuffing indicates if we have to read using byte stuffing.
 */
uint8_t read_bitstream(struct bitstream *stream,
                uint8_t nb_bits, uint32_t *dest,
                bool byte_stuffing)
{
        int8_t bit;
        uint16_t nb_bit_read = 0;
        uint32_t out = 0;

        if (stream == NULL || stream->file == NULL || dest == NULL)
                return 0;
        
        /* Read bit per bit */
        for (uint8_t i = 0; i < nb_bits; i++) {
                bit = next_bit(stream, byte_stuffing);

                if (bit >= 0)
                        out = (out << 1) | bit, nb_bit_read++;
                else
                        break;
        }

        *dest = out;

        return nb_bit_read;
}

/* Read in the stream until the value "byte" is found or the end of file */
bool skip_bitstream_until(struct bitstream *stream, uint8_t byte)
{
        if (stream != NULL && stream->file != NULL) {

                /* The current byte is our goal */
                if (stream->index == 8 && stream->byte == byte) {
                        return true;
                }
                else {
                        uint8_t *cur_byte = &stream->byte;
                        uint32_t size = 1;

                        /*
                         * Skips bytes until the value byte
                         * is found or the end of file
                         */
                        while (*cur_byte != byte && size > 0) {
                                size = fread(cur_byte, 1, 1, stream->file);
                        }

                        if (*cur_byte == byte) {
                                stream->index = 8;

                                return true;
                        }
                        else
                                stream->index = 0;
                }
        }

        return false;
}

/* Close the stream and free all memory */
void free_bitstream(struct bitstream *stream)
{
        if (stream != NULL) {
                if (stream->file != NULL)
                        fclose(stream->file);

                SAFE_FREE(stream);
        }
}

/* Write a bit into the stream */
int8_t write_bit(struct bitstream *stream, uint8_t bit, bool byte_stuffing)
{
        uint32_t size = 0;
        uint8_t *byte = &stream->byte;

        /* Save the bit in the stream buffer byte */
        *byte = *byte << 1 | (bit & 1);

        /* Write the byte buffer once full */
        if (++stream->index == 8) {

                uint8_t cur = *byte;

                size = fwrite(byte, 1, 1, stream->file);
                *byte = 0;

                /* Byte Stuffing */
                if (byte_stuffing && cur == 0xFF)
                        size = fwrite(byte, 1, 1, stream->file);

                /* Error handling */
                if (size == 0)
                        return -2;

                stream->index = 0;
        }

        return 0;
}

/* Write a byte into the stream */
void write_byte(struct bitstream *stream, uint8_t byte)
{
        fwrite(&byte, 1, 1, stream->file);
}

/* Write a short as big endian into the stream*/
void write_short_BE(struct bitstream *stream, uint16_t val)
{
        /* Convert to big endian */
        uint8_t temp[2];

        temp[0] = val >> 8;
        temp[1] = val;
        
        fwrite(temp, 1, 2, stream->file);
}

/* Seek the stream to a specific position */
void seek_bitstream(struct bitstream *stream, uint32_t pos)
{
        stream->byte = 0;
        stream->index = 0;

        fseek(stream->file, pos, SEEK_SET);
}

/* Returns the current stream position */
uint32_t pos_bitstream(struct bitstream *stream)
{
        uint32_t pos = 0;

        if (stream != NULL && stream->file != NULL)
                pos = ftell(stream->file);

        return pos;
}

/*
 * Writes all remaining bits to the stream if necessary
 */
void flush_bitstream(struct bitstream *stream)
{
        if (stream == NULL)
                return;

        if (stream->index > 0) {

                /* Add the 0 required to reach 8 bits */
                stream->byte <<= 8 - stream->index;

                /* Write the byte buffer */
                write_byte(stream, stream->byte);

                /* Reset the byte buffer */
                stream->index = 0;
                stream->byte = 0;
        }
}


