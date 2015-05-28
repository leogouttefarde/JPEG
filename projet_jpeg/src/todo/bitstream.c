
#include "bitstream.h"

struct bitstream {
        FILE *file;
        FILE *file;
};


struct bitstream *create_bitstream(const char *filename)
{
        return NULL;
}

bool end_of_bitstream(struct bitstream *stream)
{
        return false;
}

uint8_t read_bitstream(struct bitstream *stream,
                uint8_t nb_bits, uint32_t *dest,
                bool byte_stuffing);
{
        return 0;
}

void skip_bitstream_until(struct bitstream *stream
                uint8_t byte);
{
        
}

void free_bitstream(struct bitstream *stream)
{
        
}

