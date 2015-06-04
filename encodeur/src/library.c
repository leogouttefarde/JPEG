
#include "library.h"
#include <unistd.h>
#include <getopt.h>


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
                        printf("%2u, ", block[i * BLOCK_DIM + j]);

                printf("\n");
        }
        
        printf("\n\n");

        return error;
}

bool print_byte_block(uint8_t *block)
{
        bool error = true;

        for (uint8_t i = 0; i < BLOCK_DIM; i++) {
                for (uint8_t j = 0; j < BLOCK_DIM; j++)
                        printf("%2u, ", block[i * BLOCK_DIM + j]);

                printf("\n");
        }
        
        printf("\n\n");

        return error;
}

bool is_valid_jpeg(char *path)
{
        bool result = false;

        if (path != NULL) {
                char *dot = strrchr(path, '.');

                if (dot++ != NULL) {
                        if (!strcasecmp(dot, "jpg") || !strcasecmp(dot, "jpeg"))
                                result = true;
                }
        }

        return result;
}

bool is_valid_tiff(char *path)
{
        bool result = false;

        if (path != NULL) {
                char *dot = strrchr(path, '.');

                if (dot++ != NULL) {
                        if (!strcasecmp(dot, "tiff") || !strcasecmp(dot, "tif"))
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

uint32_t truncate(int32_t s)
{
        s = (s > 255) ? 255 : ( (s < 0) ? 0 : s );

        return (uint32_t)s;
}

uint8_t double2uint8(double x)
{
        uint8_t res;

        res = (x > 255) ? 255 : ( (x < 0) ? 0 : (uint8_t)x );

        return res;
}

uint32_t get_value(char *str, bool *error)
{
        uint32_t val;
        char *endptr = NULL;

        if (str != NULL)
                val = strtol(str, &endptr, 10);
        else
                *error = true;

        if (endptr == str)
                *error = true;

        return val;
}


uint8_t affect_mcu(uint32_t val, bool *error)
{
        uint8_t dim = BLOCK_DIM;

        switch(val) {
                case 16:
                case 8:
                        dim = val;
                        break;

                default:
                        *error = true;
        }

        return dim;
}

bool parse_args(int argc, char **argv, struct options *options)
{
        bool error = false;

        bool gray = false;
        uint8_t mcu_h = DEFAULT_MCU_WIDTH;
        uint8_t mcu_v = DEFAULT_MCU_HEIGHT;


        uint8_t quality = DEFAULT_COMPRESSION;
        char *path = NULL;
        char *dest = NULL;

        int c;
        char *copt = NULL;
        char *mopt = NULL;

        /* Disable default warnings */
        opterr = 0;

        while ( (c = getopt(argc, argv, "o:c:m:g")) != -1) {

                // Pour détecter arguments doubles
                // int this_option_optind = optind ? optind : 1;

                switch (c) {
                        case 'o':
                                dest = optarg;
                                // printf ("Fichier de sortie : '%s'\n", optarg);
                                break;

                        case 'c':
                                copt = optarg;
                                // printf ("Compression [0-25] : '%s'\n", optarg);
                                break;

                        case 'm':
                                mopt = optarg;
                                // printf ("Taille des MCUs : '%s'\n", optarg);
                                break;

                        case 'g':
                                gray = true;
                                break;

                        // default:
                                // printf ("Unrecognized option : %c\n", c);
                }
        }


        if (copt != NULL) {
                uint32_t val = get_value(copt, &error);

                if (!error) {
                        if (0 <= val && val <= 25) {
                                quality = val;
                                // printf("Compression : %d\n", quality);
                        }
                }
        }

        if (mopt != NULL) {
                uint32_t h_val;
                uint32_t v_val;

                h_val = get_value(mopt, &error);
                char *next = strchr(mopt, 'x');

                if (next++ != NULL)
                        v_val = get_value(next, &error);

                else
                        error = true;

                if (!error) {
                        mcu_h = affect_mcu(h_val, &error);
                        mcu_v = affect_mcu(v_val, &error);

                        printf("New MCUs : h=%d v=%d\n", mcu_h, mcu_v);
                }
        }



        if (optind < argc) {

                // printf("optind = %u\n", optind);
                path = argv[optind];
                // printf("path = %s\n", path);
                optind++;

                if (optind < argc) {
                        printf ("Paramètres non reconnus : ");
                        while (optind < argc)
                                printf ("%s ", argv[optind++]);

                        printf ("\n");
                }
        }


        if (path == NULL || dest == NULL) {
                error = true;
        }


        if (!error && !is_valid_jpeg(path) && !is_valid_tiff(path)) {
                printf("ERROR : Invalid input file extension, .tiff .tif .jpg or .jpeg expected\n");
                error = true;
        } else if (error)
                printf(USAGE, argv[0]);



        options->input = path;
        options->output = dest;

        options->mcu_h = mcu_h;
        options->mcu_v = mcu_v;

        options->compression = quality;
        options->gray = gray;


        return error;
}


