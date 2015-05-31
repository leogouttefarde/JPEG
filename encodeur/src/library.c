
#include "library.h"


bool read_short_BE(struct bitstream *stream, uint16_t *value)
{
        bool error = true;

        if (stream != NULL && value != NULL) {
                uint8_t count;
                uint32_t dest;

                count = read_bitstream(stream, 16, &dest, false);

                if (count == 16) {
                        error = false;
                        *value = (uint16_t)dest;
                }
        }

        return error;
}

bool read_byte(struct bitstream *stream, uint8_t *value)
{
        bool error = true;

        if (stream != NULL && value != NULL) {
                uint8_t count;
                uint32_t dest;

                count = read_bitstream(stream, 8, &dest, false);

                if (count == 8) {
                        error = false;
                        *value = (uint8_t)dest;
                }
        }

        return error;
}

bool print_block(int32_t *block)
{
        bool error = true;

        for (uint8_t i = 0; i < BLOCK_DIM; i++) {
                for (uint8_t j = 0; j < BLOCK_DIM; j++)
                        printf("%u ", block[i * BLOCK_DIM + j]);

                printf("\n");
        }
        
        printf("\n\n");

        return error;
}

bool is_valid_ext(char *path)
{
        bool result = false;

        if (path != NULL) {
                char *dot = strrchr(path, '.') + 1;

                if (dot != NULL) {
                        if (!strcasecmp(dot, "jpg") || !strcasecmp(dot, "jpeg"))
                                result = true;
                }
        }

        return result;
}

bool skip_bitstream(struct bitstream *stream, uint32_t nb_bytes)
{
        bool error = false;

        if (stream != NULL) {
                uint8_t byte;

                for (uint32_t i = 0; i < nb_bytes; i++)
                        error |= read_byte(stream, &byte);
        }

        return error;
}


