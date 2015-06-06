
#include "decode.h"
#include "huffman.h"
#include "unpack.h"
#include "iqzz.h"
#include "idct.h"
#include "tiff.h"
#include "conv.h"
#include "upsampler.h"
#include "downsampler.h"
#include "library.h"


static void read_jpeg(struct jpeg_data *ojpeg, bool *error);

static void read_tiff(struct jpeg_data *ojpeg, bool *error);

static void scan_jpeg(struct bitstream *stream, struct jpeg_data *jpeg, bool *error);

static const uint8_t generic_qt[64];


/* Extract raw image data */
void read_image(struct jpeg_data *jpeg, bool *error)
{
        if (jpeg == NULL || jpeg->path == NULL || *error) {
                *error = true;
                return;
        }

        if (is_valid_jpeg(jpeg->path))
                read_jpeg(jpeg, error);

        else if (is_valid_tiff(jpeg->path))
                read_tiff(jpeg, error);

        else
                *error = true;


        if (!*error) {

                uint8_t *Y_qtable = (uint8_t*)&jpeg->qtables[0];
                // uint8_t *CbCr_qtable = (uint8_t*)&jpeg->qtables[1];
                uint8_t i_q = 0;

                /* Initialize table indexes */
                for (uint8_t i = 0; i < jpeg->nb_comps; i++) {

                        /* Only use the first AC/DC and QT tables */
                        jpeg->comps[i].i_dc = 0;
                        jpeg->comps[i].i_ac = 0;


                        //if (i > 0)
                        //        i_q = 1;

                        jpeg->comps[i].i_q = i_q;
                }

                // print_byte_block(stat_qt);
                quantify_qtable(Y_qtable, generic_qt, jpeg->compression);
                // compute_qtable(CbCr_qtable, stat_qt, 3);
                // compute_qtable(Y_qtable, Y_gimp, 2);
                // compute_qtable(CbCr_qtable, CbCr_gimp, 2);


                /* Initialize components's SOS order */
                for (uint8_t i = 0; i < MAX_COMPS; i++)
                        jpeg->comp_order[i] = i;
        }
}

/* Extract and decode a JPEG file */
static void read_jpeg(struct jpeg_data *ojpeg, bool *error)
{
        if (ojpeg == NULL || ojpeg->path == NULL || *error)
                *error = true;

        else {
                struct jpeg_data jpeg;
                memset(&jpeg, 0, sizeof(jpeg));


                struct bitstream *stream = create_bitstream(ojpeg->path, RDONLY);

                if (stream != NULL) {

                        /* Read header data */
                        read_header(stream, &jpeg, error);
                        ojpeg->nb_comps = jpeg.nb_comps;


                        detect_mcu(&jpeg, error);


                        ojpeg->mcu.h = jpeg.mcu.h;
                        ojpeg->mcu.v = jpeg.mcu.v;

                        ojpeg->height = jpeg.height;
                        ojpeg->width = jpeg.width;

                        compute_mcu(ojpeg, error);


                        scan_jpeg(stream, &jpeg, error);

                        ojpeg->raw_mcu = jpeg.raw_mcu;


                        free_bitstream(stream);
                        free_jpeg_data(&jpeg);

                } else
                        *error = true;
        }
}

/* Read header data */
void read_header(struct bitstream *stream, struct jpeg_data *jpeg, bool *error)
{
        uint8_t marker = ANY;

        if (stream == NULL || error == NULL || *error || jpeg == NULL)
                return;

        /* SOI check */
        marker = read_section(stream, SOI, NULL, error);

        if (marker != SOI)
                printf("ERROR : all JPEG files must start with an SOI section\n");

        while (!*error && marker != SOS) {

                marker = read_section(stream, ANY, jpeg, error);
                *error |= end_of_bitstream(stream);
        }
}

/* Read a section */
uint8_t read_section(struct bitstream *stream, enum jpeg_section section,
                        struct jpeg_data *jpeg, bool *error)
{
        uint8_t byte;
        uint8_t marker = ANY;

        if (stream == NULL || error == NULL || *error)
                return marker;


        /* Check for SECTION_HEAD */
        *error |= read_byte(stream, &byte);
        // assert(byte == SECTION_HEAD);

        if (byte != SECTION_HEAD)
                *error = true;

        /* Retrieve section marker */
        *error |= read_byte(stream, &marker);

        if (section && section != marker)
                *error = true;

        if (marker == SOI || marker == EOI)
                return marker;


        uint16_t size;
        int32_t unread;

        /* Read section size */
        *error |= read_short_BE(stream, &size);
        // printf("size = %04lX\n", size);
        unread = size - sizeof(size);


        /* Process each section */
        switch (marker) {

        case APP0:
                if (!*error) {
                        char jfif[5];

                        for (uint8_t i = 0; i < sizeof(jfif); i++) {
                                *error |= read_byte(stream, (uint8_t*)&jfif[i]);
                                unread--;
                        }

                        /* JFIF header check */
                        if (strcmp(jfif, "JFIF"))
                                *error = true;
                }

                /* Skip unused header data */
                skip_bitstream(stream, unread);
                break;

        case COM:
                /* Skip the comment section */
                skip_bitstream(stream, unread);
                break;

        case DQT:
                if (jpeg != NULL) {

                        /* Read all quantification tables */
                        do {
                                read_byte(stream, &byte);
                                unread--;

                                bool accuracy = ((uint8_t)byte) >> 4;

                                if (accuracy) {
                                        printf("ERROR : this baseline JPEG decoder only supports 8 bits accuracy\n");
                                        *error = true;
                                }

                                else {
                                        // printf("unread = %d\n", unread);
                                        uint8_t i_q = byte & 0xF;

                                        if (i_q < MAX_QTABLES) {
                                                uint8_t *qtable = (uint8_t*)&jpeg->qtables[i_q];

                                                // printf("unread = %d\n", unread);
                                                for (uint8_t i = 0; i < BLOCK_SIZE; i++) {
                                                        *error |= read_byte(stream, &qtable[i]);
                                                        unread--;
                                                }
                                        } else
                                                *error = true;

                                        // printf("unread = %d\n", unread);
                                        jpeg->state |= DQT_OK;
                                }

                        } while (unread > 0 && !*error);
                } else
                        *error = true;

                break;

        case SOF0:
                if (jpeg != NULL) {
                        uint8_t accuracy;
                        read_byte(stream, &accuracy);

                        if (accuracy != 8) {
                                printf("ERROR : this baseline JPEG decoder only supports 8 bits accuracy\n");
                                *error = true;
                        }


                        read_short_BE(stream, &jpeg->height);
                        read_short_BE(stream, &jpeg->width);

                        read_byte(stream, &jpeg->nb_comps);


                        /* Only RGB & Gray JPEG images are possible */
                        if (jpeg->nb_comps != 3
                            && jpeg->nb_comps != 1) {
                                *error = true;
                        } else {
                                for (uint8_t i = 0; i < jpeg->nb_comps; i++) {
                                        uint8_t i_c, i_q;
                                        uint8_t h_sampling_factor;
                                        uint8_t v_sampling_factor;

                                        *error |= read_byte(stream, &i_c);

                                        if (i_c < 1 || i_c > 3)
                                                *error = true;

                                        else
                                                --i_c;

                                        *error |= read_byte(stream, &byte);
                                        h_sampling_factor = byte >> 4;
                                        v_sampling_factor = byte & 0xF;

                                        *error |= read_byte(stream, &i_q);

                                        if (i_q >= MAX_QTABLES)
                                                *error = true;

                                        if (!*error) {
                                                jpeg->comps[i_c].nb_blocks_h = h_sampling_factor;
                                                jpeg->comps[i_c].nb_blocks_v = v_sampling_factor;
                                                jpeg->comps[i_c].i_q = i_q;
                                        }
                                        //printf("nb_blocks_h = %d\n", h_sampling_factor);
                                        //printf("nb_blocks_v = %d\n", v_sampling_factor);
                                        jpeg->state |= SOF0_OK;
                                }
                        }
                } else
                        *error = true;

                break;

        case DHT:
                if (jpeg != NULL) {
                        uint8_t unused, type, i_h;

                        /* Read all Huffman tables */
                        while (unread > 0 && !*error) {

                                *error |= read_byte(stream, &byte);
                                unread--;

                                unused = byte >> 5;

                                /*
                                 * 0 for DC
                                 * 1 for AC
                                 */
                                type = (byte >> 4) & 1;

                                i_h = byte & 0xF;


                                /*
                                 * Unused must always be zero
                                 * Never more than 4 tables for each AC/DC type
                                 */
                                if (unused || i_h > 3 || type > 1)
                                        *error = true;


                                uint16_t nb_byte_read;
                                struct huff_table *table;

                                table = load_huffman_table(stream, &nb_byte_read);

                                if (nb_byte_read == -1 || table == NULL)
                                        *error = true;


                                if (!*error)
                                        jpeg->htables[type][i_h] = table;
                                else
                                        SAFE_FREE(table);

                                unread -= nb_byte_read;
                                // printf("unread = %i\n", unread);
                                jpeg->state |= DHT_OK;
                        }
                } else
                        *error = true;

                break;

        /* Start Of Scan */
        case SOS:
                if (jpeg != NULL) {
                        uint8_t nb_comps, i_c;


                        read_byte(stream, &nb_comps);

                        if (nb_comps != jpeg->nb_comps) {
                                *error = true;
                                return SOS;
                        }

                        for (uint8_t i = 0; i < nb_comps; i++) {
                                read_byte(stream, &byte);
                                i_c = --byte;

                                jpeg->comp_order[i] = i_c;
                                // printf("i_c = %i\n", byte);

                                read_byte(stream, &byte);
                                jpeg->comps[i_c].i_dc = byte >> 4;
                                jpeg->comps[i_c].i_ac = byte & 0xF;
                        }

                        read_byte(stream, &byte);
                        read_byte(stream, &byte);
                        read_byte(stream, &byte);
                } else
                        *error = true;

                break;

        // case TEM:
        // case DNL:
        // case DHP:
        // case EXP:
        default:
                printf("Unsupported marker : %02X\n", marker);
                skip_bitstream(stream, unread);
        }


        return marker;
}

/* Read a TIFF file's image data */
static void read_tiff(struct jpeg_data *ojpeg, bool *error)
{
        if (ojpeg == NULL || ojpeg->path == NULL || *error)
                *error = true;

        else {
                struct tiff_file_desc *file = NULL;
                uint32_t width, height, row_per_strip;

                file = init_tiff_file_read(ojpeg->path, &width, &height, &row_per_strip);

                if (file != NULL) {

                        ojpeg->nb_comps = 3;

                        ojpeg->width = width;
                        ojpeg->height = height;

                        if (row_per_strip == 8 || row_per_strip == 16) {
                                ojpeg->mcu.v = row_per_strip;
                                compute_mcu(ojpeg, error);
                        } else if (row_per_strip == 0) {
                                ojpeg->mcu.h = width;
                                ojpeg->mcu.v = height;
                                ojpeg->mcu.h_dim = 1;
                                ojpeg->mcu.v_dim = 1;
                                ojpeg->mcu.nb = 1;
                                ojpeg->mcu.nb_h = 1;
                                ojpeg->mcu.nb_v = 1;
                                ojpeg->mcu.size = width * height;
                        } else
                                *error = true;


                        if (!*error) {
                                uint32_t *mcu_RGB = NULL;

                                const uint32_t nb_pixels_max = ojpeg->mcu.size * ojpeg->mcu.nb;
                                ojpeg->raw_mcu = malloc(nb_pixels_max * sizeof(uint32_t));
                                // printf("nb_pixels_max = %u\n", nb_pixels_max);


                                if (ojpeg->raw_mcu == NULL)
                                        *error = true;

                                else {
                                        for (uint32_t i = 0; i < ojpeg->mcu.nb; i++) {

                                                mcu_RGB = &(ojpeg->raw_mcu[i * ojpeg->mcu.size]);

                                                read_tiff_file(file, mcu_RGB, ojpeg->mcu.h_dim, ojpeg->mcu.v_dim);
                                        }
                                }
                        }

                        close_tiff_file(file);

                } else
                        *error = true;
        }
}


/* Extract and decode raw JPEG data */
static void scan_jpeg(struct bitstream *stream, struct jpeg_data *jpeg, bool *error)
{
        if (stream == NULL || error == NULL || *error || jpeg == NULL)
                return;

        uint8_t i_c;

        uint8_t mcu_h = jpeg->mcu.h;
        uint8_t mcu_v = jpeg->mcu.v;
        uint8_t mcu_h_dim = jpeg->mcu.h_dim;
        uint8_t mcu_v_dim = jpeg->mcu.v_dim;

        uint32_t nb_mcu = jpeg->mcu.nb;


        uint8_t nb_blocks_h, nb_blocks_v, nb_blocks;
        uint8_t i_dc, i_ac, i_q;
        int32_t *last_DC;

        uint8_t idct[mcu_h_dim * mcu_v_dim][BLOCK_SIZE];
        int32_t block[BLOCK_SIZE];
        int32_t iqzz[BLOCK_SIZE];
        uint8_t *upsampled;

        uint32_t mcu_size = mcu_h * mcu_v;
        uint32_t *mcu_RGB = NULL;
        uint8_t data_YCbCr[3][mcu_size];
        uint8_t *mcu_YCbCr[3] = {
                (uint8_t*)&data_YCbCr[0],
                (uint8_t*)&data_YCbCr[1],
                (uint8_t*)&data_YCbCr[2]
        };

        const uint32_t nb_pixels_max = mcu_size * nb_mcu;
        jpeg->raw_mcu = malloc(nb_pixels_max * sizeof(uint32_t));

        if (jpeg->raw_mcu == NULL) {
                *error = true;
                return;
        }


        // struct tiff_file_desc *file = init_tiff_file("test.tiff", jpeg->width, jpeg->height, mcu_v);


        for (uint32_t i = 0; i < nb_mcu; i++) {
                mcu_RGB = &jpeg->raw_mcu[i * mcu_size];

                for (uint8_t j = 0; j < jpeg->nb_comps; j++) {

                        i_c = jpeg->comp_order[j];

                        nb_blocks_h = jpeg->comps[i_c].nb_blocks_h;
                        nb_blocks_v = jpeg->comps[i_c].nb_blocks_v;
                        nb_blocks = nb_blocks_h * nb_blocks_v;

                        i_dc = jpeg->comps[i_c].i_dc;
                        i_ac = jpeg->comps[i_c].i_ac;
                        i_q = jpeg->comps[i_c].i_q;

                        last_DC = &jpeg->comps[i_c].last_DC;


                        for (uint8_t n = 0; n < nb_blocks; n++) {

                                unpack_block(stream, jpeg->htables[0][i_dc], last_DC,
                                                     jpeg->htables[1][i_ac], block);

                                iqzz_block(block, iqzz, (uint8_t*)&jpeg->qtables[i_q]);
                                idct_block(iqzz, (uint8_t*)&idct[n]);
                        }

                        upsampled = mcu_YCbCr[i_c];
                        upsampler((uint8_t*)idct, nb_blocks_h, nb_blocks_v, upsampled, mcu_h_dim, mcu_v_dim);
                }

                if (jpeg->nb_comps == 3) {
                        YCbCr_to_ARGB(mcu_YCbCr, mcu_RGB, mcu_h_dim, mcu_v_dim);
                }

                else if (jpeg->nb_comps == 1) {
                        Y_to_ARGB(mcu_YCbCr[0], mcu_RGB, mcu_h_dim, mcu_v_dim);
                }
                else
                        *error = true;


                // write_tiff_file(file, mcu_RGB, mcu_h_dim, mcu_v_dim);
        }

        // close_tiff_file(file);
}



/* Luminance gimp sujet */
// const uint8_t Y_gimp[64] =
// {
//         0x05, 0x03, 0x03, 0x05, 0x07, 0x0c, 0x0f, 0x12,
//         0x04, 0x04, 0x04, 0x06, 0x08, 0x11, 0x12, 0x11,
//         0x04, 0x04, 0x05, 0x07, 0x0c, 0x11, 0x15, 0x11,
//         0x04, 0x05, 0x07, 0x09, 0x0f, 0x1a, 0x18, 0x13,
//         0x05, 0x07, 0x0b, 0x11, 0x14, 0x21, 0x1f, 0x17,
//         0x07, 0x0b, 0x11, 0x13, 0x18, 0x1f, 0x22, 0x1c,
//         0x0f, 0x13, 0x17, 0x1a, 0x1f, 0x24, 0x24, 0x1e,
//         0x16, 0x1c, 0x1d, 0x1d, 0x22, 0x1e, 0x1f, 0x1e
// };

/* Chrominance gimp sujet */
// const uint8_t CbCr_gimp[64] =
// {
//         0x05, 0x05, 0x07, 0x0e, 0x1e, 0x1e, 0x1e, 0x1e,
//         0x05, 0x06, 0x08, 0x14, 0x1e, 0x1e, 0x1e, 0x1e,
//         0x07, 0x08, 0x11, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
//         0x0e, 0x14, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
//         0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
//         0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
//         0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
//         0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e
// };


/* (1 + (x + y + 1)) QTable */
static const uint8_t generic_qt[64] =
{
         2,  3,  3,  4,  4,  4,  5,  5,
         5,  5,  6,  6,  6,  6,  6,  7,
         7,  7,  7,  7,  7,  8,  8,  8,
         8,  8,  8,  8,  9,  9,  9,  9,
         9,  9,  9,  9, 10, 10, 10, 10,
        10, 10, 10, 11, 11, 11, 11, 11,
        11, 12, 12, 12, 12, 12, 13, 13,
        13, 13, 14, 14, 14, 15, 15, 16
};

/* (x + y + 1) QTable */
// const uint8_t generic_qt[64] =
// {
//          1,  2,  2,  3,  3,  3,  4,  4,
//          4,  4,  5,  5,  5,  5,  5,  6,
//          6,  6,  6,  6,  6,  7,  7,  7,
//          7,  7,  7,  7,  8,  8,  8,  8,
//          8,  8,  8,  8,  9,  9,  9,  9,
//          9,  9,  9, 10, 10, 10, 10, 10,
//         10, 11, 11, 11, 11, 11, 12, 12,
//         12, 12, 13, 13, 13, 14, 14, 15
// };

