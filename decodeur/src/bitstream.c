
#include "bitstream.h"
#include "common.h"

#define BUFFER_SIZE 16


/*
 * Internal bitstream structure
 */
struct bitstream {

        /* Currently opened file */
        FILE *file;

        /* Current byte */
        uint8_t byte;

        /* Next bit's index in byte */
        uint8_t index;

        /* Reading buffer */
        uint8_t buffer[BUFFER_SIZE];

        /* Number of bytes available in buffer */
        uint8_t buffer_size;

        /* Next byte's index in buffer */
        uint8_t buf_idx;
};


/* Open the filename file as bitstream */
struct bitstream *create_bitstream(const char *filename)
{
        struct bitstream *stream = NULL;

        /* Open the filename file as binary for reading */
        if (filename != NULL) {
                FILE *file = fopen(filename, "rb");

                /* Create and initialize the stream */
                if (file != NULL) {
                        stream = malloc(sizeof(struct bitstream));

                        if (stream != NULL) {
                                stream->file = file;
                                stream->byte = 0;
                                stream->index = 0;
                                stream->buffer_size = 0;
                                stream->buf_idx = BUFFER_SIZE;
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

/* Read the next byte in the stream */
uint8_t next_byte(struct bitstream *stream, bool *error)
{
        uint8_t byte = 0;
        size_t ret;
        
        /* Reads BUFFER_SIZE bytes in the stream */
        if (stream->buf_idx >= stream->buffer_size) {
                ret = fread(stream->buffer, 1, BUFFER_SIZE, stream->file);

                if (ret > 0) {
                        stream->buffer_size = ret;
                        stream->buf_idx = 0;
                }
                else
                        *error = true;
        }

        /* Return the next byte from the buffer */
        if (stream->buf_idx < stream->buffer_size)
                byte = stream->buffer[stream->buf_idx++];
        else
                *error = true;

        return byte;
}

/* Read the next bit from the stream */
static inline int8_t next_bit(struct bitstream *stream, bool byte_stuffing)
{
        int8_t bit;
        uint8_t *byte = &stream->byte;

        /* Read a new byte */
        if (stream->index == 0) {
                bool error = false;
                uint8_t last = *byte;

                *byte = next_byte(stream, &error);

                /* Byte stuffing */
                if (byte_stuffing && last == 0xFF) {
                        if (*byte != 0x00)
                                return -1;

                        *byte = next_byte(stream, &error);
                }

                /* Error handling */
                if (error)
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

        if (stream == NULL || stream->file == NULL || dest == NULL || !nb_bits)
                return 0;

        /* 
         * Read bit per bit if :
         * - nb_bits < 8
         * - there are still bits in stream->byte buffer
         * - nb_bits not divisible by 8
         */
        if (byte_stuffing || stream->index != 8 || nb_bits % 8) {

                /* Read each required bit */
                for (uint8_t i = 0; i < nb_bits; i++) {
                        bit = next_bit(stream, byte_stuffing);

                        if (bit >= 0)
                                out = (out << 1) | bit, nb_bit_read++;
                        else
                                break;
                }
        }

        /* Read byte per byte otherwise */
        else {
                bool error = false;

                /* Retrieve bits from byte buffer */
                out = stream->byte;

                /* Read byte per byte until nb_bits is reached */
                if (nb_bits > 8) {
                        out = out << 8 | next_byte(stream, &error);

                        if (nb_bits > 16) {
                                out = out << 8 | next_byte(stream, &error);

                                if (nb_bits > 24)
                                        out = out << 8 | next_byte(stream, &error);
                        }
                }

                stream->index = 0;

                if (!error)
                        nb_bit_read = nb_bits;
        }

        *dest = out;


        return nb_bit_read;
}

/* Read in the stream until the value "byte" is found or the end of file */
bool skip_bitstream_until(struct bitstream *stream, uint8_t byte)
{
        if (stream != NULL && stream->file != NULL) {

                /* The current byte is our goal */
                if (stream->index == 8 && stream->byte == byte)
                        return true;

                else {
                        uint8_t *cur_byte = &stream->byte;
                        bool error = false;

                        /*
                         * Skips bytes until the value byte
                         * is found or the end of file
                         */
                        while (*cur_byte != byte && !error)
                                *cur_byte = next_byte(stream, &error);

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

