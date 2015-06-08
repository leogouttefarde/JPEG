
#include "library.h"
#include "decode.h"
#include "tiff.h"
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

int32_t get_value(char *str, bool *error)
{
        int32_t val;
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
        bool encode = true;

        char *input = NULL;
        char *output = NULL;

        uint8_t quality = DEFAULT_COMPRESSION;
        uint8_t mcu_h = DEFAULT_MCU_WIDTH;
        uint8_t mcu_v = DEFAULT_MCU_HEIGHT;
        bool gray = false;


        int opt;
        char *i_comp = NULL;
        char *i_mcu = NULL;


        /* Disable default warnings */
        opterr = 0;

        while ( (opt = getopt(argc, argv, "o:c:m:ghd")) != -1) {

                switch (opt) {
                        case 'o':
                                output = optarg;
                                break;

                        case 'c':
                                i_comp = optarg;
                                break;

                        case 'm':
                                i_mcu = optarg;
                                break;

                        case 'g':
                                gray = true;
                                break;

                        case 'h':
                                error = true;
                                break;

                        case 'd':
                                encode = false;
                                break;
                }
        }


        /* Compression rate detection */
        if (i_comp != NULL) {
                int32_t val = get_value(i_comp, &error);

                if (!error) {
                        if (0 <= val && val <= 25)
                                quality = val;
                        else
                                error = true;
                }
        }

        /* New MCU size detection */
        if (i_mcu != NULL) {
                uint32_t h_val;
                uint32_t v_val;

                h_val = get_value(i_mcu, &error);
                char *next = strchr(i_mcu, 'x');

                if (next++ != NULL)
                        v_val = get_value(next, &error);

                else
                        error = true;

                if (!error) {
                        mcu_h = affect_mcu(h_val, &error);
                        mcu_v = affect_mcu(v_val, &error);
                }
        }


        /* Input image file detection (first non-optional parameter) */
        if (optind < argc) {
                input = argv[optind];
                optind++;
        }


        if (input == NULL || output == NULL)
                error = true;


        if (error)
                printf(USAGE, argv[0]);

        else if (!is_valid_jpeg(input) && !is_valid_tiff(input)) {
                printf("ERROR : Invalid input file extension, .tiff .tif .jpg or .jpeg expected\n");
                error = true;
        }


        options->input = input;
        options->output = output;

        options->mcu_h = mcu_h;
        options->mcu_v = mcu_v;

        options->compression = quality;
        options->gray = gray;
        options->encode = encode;


        return error;
}

uint32_t *mcu_to_image(
        uint32_t *data, struct mcu_info *mcu,
        uint32_t width, uint32_t height)
{
        uint32_t index;
        uint32_t pos = 0;
        uint32_t *image = malloc(width * height * sizeof(uint32_t));

        if (image == NULL)
                return NULL;


        for (uint32_t nb_v = 0; nb_v < mcu->nb_v; nb_v++)
        for (uint32_t v = 0; v < mcu->v; v++)
        for (uint32_t nb_h = 0; nb_h < mcu->nb_h; nb_h++)
        for (uint32_t h = 0; h < mcu->h; h++) {

                /* Check for image overlapping */
                if (nb_h * mcu->h + h < width) {

                        /* Additionnal image buffer check */
                        if (pos < width * height) {

                                index = (nb_v * mcu->nb_h + nb_h) * mcu->size + v * mcu->h + h;
                                image[pos++] = data[index];
                        }
                }
        }

        return image;
}

uint32_t *image_to_mcu(
        uint32_t *image, struct mcu_info *mcu,
        uint32_t width, uint32_t height)
{
        uint32_t index, pixel;
        uint32_t pos = 0;
        uint32_t *data = malloc(mcu->size * mcu->nb * sizeof(uint32_t));

        if (data == NULL)
                return NULL;


        for (uint32_t nb_v = 0; nb_v < mcu->nb_v; nb_v++)
        for (uint32_t v = 0; v < mcu->v; v++)
        for (uint32_t nb_h = 0; nb_h < mcu->nb_h; nb_h++)
        for (uint32_t h = 0; h < mcu->h; h++) {

                /*
                 * Set unused pixels to 0,
                 * maximizing compression
                 */
                pixel = 0;

                /* Check for image overlapping */
                if (nb_h * mcu->h + h < width) {

                        /* Additionnal image buffer check */
                        if (pos < width * height)
                                pixel = image[pos++];
                }

                index = (nb_v * mcu->nb_h + nb_h) * mcu->size + v * mcu->h + h;
                data[index] = pixel;
        }

        return data;
}

void process_options(struct options *options, struct jpeg_data *jpeg, bool *error)
{
        if (options == NULL || jpeg == NULL || error == NULL || *error)
                return;


        if (options->gray) {
                if (options->encode) {
                        jpeg->nb_comps = 1;

                        options->mcu_h = BLOCK_DIM;
                        options->mcu_v = BLOCK_DIM;
                } else
                        compute_gray(jpeg);
        }


        if (jpeg->mcu.h != options->mcu_h
                || jpeg->mcu.v != options->mcu_v
                || jpeg->is_plain_image) {

                uint32_t *image = jpeg->raw_data;

                if (!jpeg->is_plain_image) {
                        image = mcu_to_image(jpeg->raw_data,
                                                &jpeg->mcu,
                                                jpeg->width,
                                                jpeg->height);

                        SAFE_FREE(jpeg->raw_data);
                }


                jpeg->mcu.h = options->mcu_h;
                jpeg->mcu.v = options->mcu_v;

                compute_mcu(jpeg, error);


                uint32_t *data = image_to_mcu(image,
                                                &jpeg->mcu,
                                                jpeg->width,
                                                jpeg->height);

                SAFE_FREE(image);

                jpeg->raw_data = data;
        }
}

void export_tiff(struct jpeg_data *jpeg, bool *error)
{
        if (*error || jpeg == NULL) {
                *error = true;
                return;
        }

        struct tiff_file_desc *file = NULL;

        file = init_tiff_file(jpeg->path, jpeg->width, jpeg->height, jpeg->mcu.v);

        if (file != NULL) {
                uint32_t *mcu_RGB;

                for (uint32_t i = 0; i < jpeg->mcu.nb; i++) {
                        mcu_RGB = &jpeg->raw_data[i * jpeg->mcu.size];
                        write_tiff_file(file, mcu_RGB, jpeg->mcu.h_dim, jpeg->mcu.v_dim);
                }

                close_tiff_file(file);

        } else
                *error = true;
}

void compute_gray(struct jpeg_data *jpeg)
{
        if (jpeg == NULL || jpeg->raw_data == NULL)
                return;


        uint32_t nb_pixels, pixel;
        uint32_t *image = jpeg->raw_data;
        uint32_t R, G, B, gray;

        if (jpeg->is_plain_image)
                nb_pixels = jpeg->width * jpeg->height;

        else
                nb_pixels = jpeg->mcu.size * jpeg->mcu.nb;


        for (uint32_t i = 0; i < nb_pixels; i++) {

                pixel = image[i];

                R = RED(pixel);
                G = GREEN(pixel);
                B = BLUE(pixel);

                gray = (R + G + B) / 3;


                image[i] = gray << 16 | gray << 8 | gray;
        }
}

