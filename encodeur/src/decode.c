
#include "decode.h"
#include "huffman.h"
#include "unpack.h"
#include "qzz.h"
#include "dct.h"
#include "tiff.h"
#include "conv.h"
#include "upsampler.h"
#include "downsampler.h"
#include "library.h"


/* Extract and decode a whole JPEG file */
static void read_jpeg(struct jpeg_data *ojpeg, bool *error);

/* Read a TIFF file's image data */
static void read_tiff(struct jpeg_data *ojpeg, bool *error);

/* Extract and decode raw JPEG data */
static void scan_jpeg(struct bitstream *stream, struct jpeg_data *jpeg, bool *error);

/*
 * Generic quantification table
 * Source : http://www-ljk.imag.fr/membres/Valerie.Perrier/SiteWeb/node10.html
 */
static const uint8_t generic_qt[64];


/* Extract raw image data */
void read_image(struct jpeg_data *jpeg, bool *error)
{
        if (jpeg == NULL || jpeg->path == NULL || *error) {
                *error = true;
                return;
        }

        /* Read input jpeg file */
        if (is_valid_jpeg(jpeg->path))
                read_jpeg(jpeg, error);

        /* Read input tiff file */
        else if (is_valid_tiff(jpeg->path))
                read_tiff(jpeg, error);

        else
                *error = true;

        /* 
         * Initialize jpeg quantification tables
         * and component informations
         */
        if (!*error) {
                uint8_t i_q = 0;
                uint8_t *qtable = (uint8_t*)&jpeg->qtables[0];

                /* Gimp QTables codes (disabled) */
                // uint8_t *Y_qtable = (uint8_t*)&jpeg->qtables[0];
                // uint8_t *CbCr_qtable = (uint8_t*)&jpeg->qtables[1];


                /* Initialize table indexes */
                for (uint8_t i = 0; i < jpeg->nb_comps; i++) {

                        /* Use one AC/DC tree per component */
                        jpeg->comps[i].i_dc = i;
                        jpeg->comps[i].i_ac = i;


                        /* Gimp QTables codes (disabled) */
                        //if (i > 0)
                        //        i_q = 1;

                        jpeg->comps[i].i_q = i_q;
                }

                quantify_qtable(qtable, generic_qt, jpeg->compression);

                /* Gimp QTables codes (disabled) */
                // quantify_qtable(Y_qtable, Y_gimp, jpeg->compression);
                // quantify_qtable(CbCr_qtable, CbCr_gimp, jpeg->compression);


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

                        /* Read jpeg header data */
                        read_header(stream, &jpeg, error);
                        ojpeg->nb_comps = jpeg.nb_comps;
                        
                        /* Detect MCU informations from the jpeg structure */
                        detect_mcu(&jpeg, error);

                        /* Initialize output information */
                        ojpeg->mcu.h = jpeg.mcu.h;
                        ojpeg->mcu.v = jpeg.mcu.v;
                        ojpeg->height = jpeg.height;
                        ojpeg->width = jpeg.width;

                        /*
                         * Compute MCU informations :
                         * Number of MCUs, size, Y / Cb / Cr dimensions
                         */
                        compute_mcu(ojpeg, error);


                        /* Extract and decode raw JPEG data */
                        scan_jpeg(stream, &jpeg, error);

                        ojpeg->raw_data = jpeg.raw_data;


                        free_bitstream(stream);
                        free_jpeg_data(&jpeg);


                        if (*error)
                                printf("ERROR : invalid input JPEG file\n");

                } else
                        *error = true;
        }
}

/* Read a whole JPEG header */
void read_header(struct bitstream *stream, struct jpeg_data *jpeg, bool *error)
{
        uint8_t marker = ANY;

        if (stream == NULL || error == NULL || *error || jpeg == NULL)
                return;

        /* SOI check */
        marker = read_section(stream, SOI, NULL, error);

        if (marker != SOI)
                printf("ERROR : all JPEG files must start with an SOI section\n");

        /* Read all sections until SOS is reached */
        while (!*error && marker != SOS) {
                marker = read_section(stream, ANY, jpeg, error);
                *error |= end_of_bitstream(stream);
        }
}

/* Read a jpeg section */
uint8_t read_section(struct bitstream *stream, enum jpeg_section section,
                        struct jpeg_data *jpeg, bool *error)
{
        uint8_t byte;
        uint8_t marker = ANY;

        if (stream == NULL || error == NULL || *error)
                return marker;


        /* Check for SECTION_HEAD 0xFF */
        *error |= read_byte(stream, &byte);

        if (byte != SECTION_HEAD)
                *error = true;

        /* Retrieve section marker */
        *error |= read_byte(stream, &marker);

        /* If the read marker is not the right one, error */
        if (section && section != marker)
                *error = true;

        /* Nothing to do when SOI or EOI */
        if (marker == SOI || marker == EOI)
                return marker;


        uint16_t size;
        int32_t unread;

        /* Read section size */
        *error |= read_short_BE(stream, &size);

        /*
         * Number of bytes to skip (unused bytes)
         * Updated when reading
         */
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
                                
                                /* Baseline accuracy is 0 */
                                bool accuracy = ((uint8_t)byte) >> 4;

                                if (accuracy) {
                                        printf("ERROR : this baseline JPEG decoder"\
                                               " only supports 8 bits accuracy\n");
                                        *error = true;
                                } else {
                                        uint8_t i_q = byte & 0xF;

                                        /*
                                         * Read one quantification table
                                         */
                                        if (i_q < MAX_QTABLES) {
                                                uint8_t *qtable = (uint8_t*)&jpeg->qtables[i_q];

                                                for (uint8_t i = 0; i < BLOCK_SIZE; i++) {
                                                        *error |= read_byte(stream, &qtable[i]);
                                                        unread--;
                                                }
                                        } else
                                                *error = true;

                                        /* Update jpeg status */
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
                                printf("ERROR : this baseline JPEG decoder"\
                                       " only supports 8 bits accuracy\n");
                                *error = true;
                        }

                        /* Read jpeg information */
                        read_short_BE(stream, &jpeg->height);
                        read_short_BE(stream, &jpeg->width);

                        read_byte(stream, &jpeg->nb_comps);

                        /* Only RGB & Gray JPEG images are possible */
                        if (jpeg->nb_comps != 3 && jpeg->nb_comps != 1)
                                *error = true;

                        else {
                                /* Read all component information */
                                for (uint8_t i = 0; i < jpeg->nb_comps; i++) {
                                        uint8_t i_c, i_q;
                                        uint8_t h_sampling_factor;
                                        uint8_t v_sampling_factor;

                                        *error |= read_byte(stream, &i_c);

                                        /*
                                         * Component index must range
                                         * in 1 - 3 or 0 - 2
                                         */
                                        if (i_c > 3)
                                                *error = true;

                                        /*
                                         * When indexes range in 1 - 3,
                                         * convert them to 0 - 2
                                         */
                                        if (i_c != i && i_c > 0)
                                                --i_c;

                                        *error |= read_byte(stream, &byte);
                                        h_sampling_factor = byte >> 4;
                                        v_sampling_factor = byte & 0xF;

                                        *error |= read_byte(stream, &i_q);

                                        if (i_q >= MAX_QTABLES)
                                                *error = true;

                                        /* Initialize component informations */
                                        if (!*error) {
                                                jpeg->comps[i_c].nb_blocks_h = h_sampling_factor;
                                                jpeg->comps[i_c].nb_blocks_v = v_sampling_factor;
                                                jpeg->comps[i_c].i_q = i_q;
                                        }

                                        /* Update jpeg status */
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

                                /* Read one Huffman Table */
                                uint16_t nb_byte_read;
                                struct huff_table *table;

                                table = load_huffman_table(stream, &nb_byte_read);

                                if (nb_byte_read == (uint16_t)-1 || table == NULL)
                                        *error = true;


                                if (!*error)
                                        jpeg->htables[type][i_h] = table;

                                else
                                        free_huffman_table(table);


                                unread -= nb_byte_read;
                                
                                /* Update jpeg status */
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

                        /* 
                         * Check that the number of component is 
                         * the same as in the SOF Section
                         */
                        if (nb_comps != jpeg->nb_comps) {
                                *error = true;
                                return SOS;
                        }

                        /* Read component informations */
                        for (uint8_t i = 0; i < nb_comps; i++) {
                                read_byte(stream, &byte);

                                /*
                                 * When indexes range in 1 - 3,
                                 * convert them to 0 - 2
                                 */
                                i_c = byte;
                                if(i_c != i && i_c > 0)
                                        --i_c;

                                jpeg->comp_order[i] = i_c;

                                /* Read Huffman table indexes */
                                read_byte(stream, &byte);
                                jpeg->comps[i_c].i_dc = byte >> 4;
                                jpeg->comps[i_c].i_ac = byte & 0xF;
                        }

                        /* Skip unused SOS data */
                        read_byte(stream, &byte);
                        read_byte(stream, &byte);
                        read_byte(stream, &byte);
                } else
                        *error = true;

                break;


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
                uint32_t width, height;
                
                /* Read TIFF header */
                file = init_tiff_read(ojpeg->path, &width, &height);

                if (file != NULL) {

                        /* Initialize output JPEG informations */
                        ojpeg->nb_comps = 3;
                        ojpeg->is_plain_image = true;
                        ojpeg->width = width;
                        ojpeg->height = height;

                        /* Read all raw image data */
                        if (!*error) {
                                uint32_t *line = NULL;

                                const uint32_t nb_pixels_max = ojpeg->width * ojpeg->height;
                                ojpeg->raw_data = malloc(nb_pixels_max * sizeof(uint32_t));

                                if (ojpeg->raw_data == NULL)
                                        *error = true;

                                else {
                                        /* Read all RGB lines */
                                        for (uint32_t i = 0; i < ojpeg->height; i++) {

                                                /* Read one RGB line */
                                                line = &(ojpeg->raw_data[i * ojpeg->width]);

                                                *error |= read_tiff_line(file, line);
                                        }
                                }
                        }

                        close_tiff_file(file);

                } else {
                        printf("ERROR : invalid input TIFF file\n");
                        *error = true;
                }
        }
}


/* Extract and decode raw JPEG data */
static void scan_jpeg(struct bitstream *stream, struct jpeg_data *jpeg, bool *error)
{
        if (stream == NULL || error == NULL || *error || jpeg == NULL)
                return;


        /* Retrieve MCU data */
        uint8_t mcu_h = jpeg->mcu.h;
        uint8_t mcu_v = jpeg->mcu.v;
        uint8_t mcu_h_dim = jpeg->mcu.h_dim;
        uint8_t mcu_v_dim = jpeg->mcu.v_dim;
        uint32_t nb_mcu = jpeg->mcu.nb;

        /* Scan variables */
        uint8_t nb_blocks_h, nb_blocks_v, nb_blocks;
        uint8_t i_c, i_q, i_dc, i_ac;
        int32_t *last_DC;

        uint8_t idct[mcu_h_dim * mcu_v_dim][BLOCK_SIZE];
        int32_t block[BLOCK_SIZE];
        int32_t iqzz[BLOCK_SIZE];
        uint8_t *upsampled;

        uint32_t mcu_size = mcu_h * mcu_v;
        uint32_t *mcu_RGB = NULL;
        uint8_t data_YCbCr[MAX_COMPS][mcu_size];
        uint8_t *mcu_YCbCr[MAX_COMPS] = {
                (uint8_t*)&data_YCbCr[0],
                (uint8_t*)&data_YCbCr[1],
                (uint8_t*)&data_YCbCr[2]
        };

        /* Buffer to store raw jpeg data */
        const uint32_t nb_pixels_max = mcu_size * nb_mcu;
        jpeg->raw_data = malloc(nb_pixels_max * sizeof(uint32_t));

        if (jpeg->raw_data == NULL) {
                *error = true;
                return;
        }

        /* Extract and decode all MCUs */
        for (uint32_t i = 0; i < nb_mcu; i++) {
                mcu_RGB = &jpeg->raw_data[i * mcu_size];

                /* Retrieve each component */
                for (uint8_t j = 0; j < jpeg->nb_comps; j++) {

                        /* Retrieve component informations */
                        i_c = jpeg->comp_order[j];

                        nb_blocks_h = jpeg->comps[i_c].nb_blocks_h;
                        nb_blocks_v = jpeg->comps[i_c].nb_blocks_v;
                        nb_blocks = nb_blocks_h * nb_blocks_v;

                        i_dc = jpeg->comps[i_c].i_dc;
                        i_ac = jpeg->comps[i_c].i_ac;
                        i_q = jpeg->comps[i_c].i_q;

                        last_DC = &jpeg->comps[i_c].last_DC;

                        /* Retrieve MCUs from the JPEG file */
                        for (uint8_t n = 0; n < nb_blocks; n++) {

                                /* Retrieve one block from the JPEG file */
                                unpack_block(stream, jpeg->htables[0][i_dc], last_DC,
                                                     jpeg->htables[1][i_ac], block);

                                /* Convert raw data to Y, Cb or Cr MCU data */
                                iqzz_block(block, iqzz, (uint8_t*)&jpeg->qtables[i_q]);
                                idct_block(iqzz, (uint8_t*)&idct[n]);
                        }

                        /* Upsample current MCUs */
                        upsampled = mcu_YCbCr[i_c];
                        upsampler((uint8_t*)idct, nb_blocks_h, nb_blocks_v, 
                                  upsampled, mcu_h_dim, mcu_v_dim);
                }

                /* Convert YCbCr to RGB for color images */
                if (jpeg->nb_comps == 3)
                        YCbCr_to_ARGB(mcu_YCbCr, mcu_RGB, mcu_h_dim, mcu_v_dim);

                /* Convert Y to RGB for grayscale images */
                else if (jpeg->nb_comps == 1)
                        Y_to_ARGB(mcu_YCbCr[0], mcu_RGB, mcu_h_dim, mcu_v_dim);

                else
                        *error = true;
        }
}


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


/* Subject's gimp Luminance table */
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

/* Subject's gimp Chrominance table */
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

