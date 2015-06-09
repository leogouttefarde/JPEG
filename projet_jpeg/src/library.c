
#include "library.h"
#include <unistd.h>
#include <getopt.h>

/*
 * Reads a short from stream as Big Endian
 */
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

/*
 * Reads a byte from stream
 */
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

/*
 * Prints a BLOCK_DIMxBLOCK_DIM block of int32_t for debug puposes.
 */
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

/*
 * Checks that the filename extension is "jpg" or "jpeg"
 */
bool is_valid_ext(char *path)
{
        bool result = false;

        if (path != NULL) {
                const char *dot = strrchr(path, '.');

                if (dot++ != NULL) {
                        if (!strcasecmp(dot, "jpg") || !strcasecmp(dot, "jpeg"))
                                result = true;
                }
        }

        return result;
}

/*
 * Skips nb_bytes bytes in the stream
 */
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

/*
 * Generates the tiff destination file path
 */
static char *create_tiff_name(char *path)
{
        if (path == NULL)
                return NULL;

        char *name, *dot;
        uint32_t len_cpy;
        uint32_t len_tiff;

        dot = strrchr(path, '.');

        /*
         * Replace the previous extension if any,
         * else add an extension.
         */
        if (dot != NULL)
                len_cpy = (uint32_t)(dot - path);
        else
                len_cpy = strlen(path);

        /* Compute the path size */
        len_tiff = len_cpy + 5 + 1;

        name = malloc(len_tiff);

        /* Create the path */
        if (name != NULL) {
                strncpy(name, path, len_cpy);
                name[len_cpy] = 0;

                strcat(name, ".tiff");
        }

        return name;
}

/*
 * Parses arguments given to the program and puts them in *options
 */
bool parse_args(int argc, char **argv, struct options *options)
{
        bool error = false;

        char *input = NULL;
        char *output = NULL;


        int opt;

        /* Disable default warnings */
        opterr = 0;

        /* Parse all arguments */
        while ( (opt = getopt(argc, argv, "o:h")) != -1) {

                switch (opt) {
                        case 'o':
                                output = optarg;
                                break;

                        case 'h':
                                error = true;
                                break;
                }
        }

        /* Input image file detection (first non-optional parameter) */
        if (optind < argc) {
                input = argv[optind];
                optind++;
        }

        /* If no input file, error */
        if (input == NULL)
                error = true;

        /* Show the help on error */
        if (error)
                printf(USAGE, argv[0]);

        else if (!is_valid_ext(input)) {
                printf("ERROR : Invalid file extension, .jpg or .jpeg expected\n");
                error = true;
        }


        options->input = input;

        /* Compute the output TIFF path */
        if (!error) {
                if (output == NULL)
                        options->output = create_tiff_name(input);

                else {
                        /* Reallocate output path */
                        const uint32_t size = strlen(output) + 1;
                        options->output = malloc(size);

                        if (options->output != NULL)
                                strncpy(options->output, output, size);

                        else
                                error = true;
                }
        } else
                options->output = NULL;


        return error;
}


