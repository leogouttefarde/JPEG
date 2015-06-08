
#include "bitstream.h"
#include "common.h"


struct bitstream {
        FILE *file;
        enum stream_mode mode;
        uint8_t byte;
        uint8_t index;
};


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


                FILE *file = fopen(filename, open_mode);

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

bool end_of_bitstream(struct bitstream *stream)
{
        bool end = true;

        if (stream != NULL && stream->file != NULL) {
                end = feof(stream->file) ? true : false;
        }

        return end;
}

int8_t next_bit(struct bitstream *stream, bool byte_stuffing)
{
        uint32_t size = 0;
        int8_t bit;
        uint8_t *byte = &stream->byte;

        if (stream->index == 0) {

                uint8_t last = *byte;
                size = fread(byte, 1, 1, stream->file);

                if (byte_stuffing && last == 0xFF) {
                        if (*byte != 0x00)
                                return -1;

                        size = fread(byte, 1, 1, stream->file);
                }

                if (size == 0)
                        return -2;

                stream->index = 8;
        }

        bit = 1 & (*byte >> --stream->index);

        return bit;
}

uint8_t read_bitstream(struct bitstream *stream,
                uint8_t nb_bits, uint32_t *dest,
                bool byte_stuffing)
{
        int8_t bit;
        uint16_t nb_bit_read = 0;
        uint32_t out = 0;

        if (stream == NULL || stream->file == NULL || dest == NULL)
                return 0;

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

bool skip_bitstream_until(struct bitstream *stream, uint8_t byte)
{
        if (stream != NULL && stream->file != NULL) {
                if (stream->index == 8 && stream->byte == byte) {
                        return true;
                }
                else {
                        uint8_t *cur_byte = &stream->byte;
                        uint32_t size = 1;

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

void free_bitstream(struct bitstream *stream)
{
        if (stream != NULL) {
                if (stream->file != NULL)
                        fclose(stream->file);

                SAFE_FREE(stream);
        }
}

int8_t write_bit(struct bitstream *stream, uint8_t bit, bool byte_stuffing)
{
        uint32_t size = 0;
        uint8_t *byte = &stream->byte;

        *byte = *byte << 1 | (bit & 1);


        if (++stream->index == 8) {

                uint8_t cur = *byte;

                size = fwrite(byte, 1, 1, stream->file);
                *byte = 0;

                if (byte_stuffing && cur == 0xFF)
                        size = fwrite(byte, 1, 1, stream->file);


                if (size == 0)
                        return -2;

                stream->index = 0;
        }

        return 0;
}

void write_byte(struct bitstream *stream, uint8_t byte)
{
        uint8_t temp = byte;

        fwrite(&temp, 1, 1, stream->file);
}

void write_short_BE(struct bitstream *stream, uint16_t val)
{
        uint8_t temp[2];

        temp[0] = val >> 8;
        temp[1] = val;

        fwrite(temp, 2, 1, stream->file);
}

void seek_bitstream(struct bitstream *stream, uint32_t pos)
{
        stream->byte = 0;
        stream->index = 0;

        fseek(stream->file, pos, SEEK_SET);
}

uint32_t pos_bitstream(struct bitstream *stream)
{
        uint32_t pos = 0;

        if (stream != NULL && stream->file != NULL)
                pos = ftell(stream->file);

        return pos;
}

void flush_bitstream(struct bitstream *stream)
{
        if (stream == NULL)
                return;

        if (stream->index > 0) {
                stream->byte <<= 8 - stream->index;

                fwrite(&stream->byte, 1, 1, stream->file);

                stream->index = 0;
                stream->byte = 0;
        }
}


