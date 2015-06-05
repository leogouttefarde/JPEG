
#include "jpeg.h"
#include "huffman.h"
#include "unpack.h"
#include "iqzz.h"
#include "idct.h"
#include "tiff.h"
#include "conv.h"
#include "upsampler.h"
#include "downsampler.h"
#include "library.h"


static inline uint16_t mcu_per_dim(uint8_t mcu, uint16_t dim);
void detect_mcu(struct jpeg_data *jpeg, bool *error);
void compute_mcu(struct jpeg_data *jpeg, bool *error);

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

char* create_tiff_name(char *path)
{
        if (path == NULL)
                return NULL;

        char *name, *dot;
        uint32_t len_cpy;
        uint32_t len_tiff;


        dot = strrchr(path, '.');

        if (dot != NULL)
                len_cpy = (uint32_t)(dot - path);
        else
                len_cpy = strlen(path);

        len_tiff = len_cpy + 5 + 1;

        // printf("len_cpy = %d\n", len_tiff);
        // printf("len_tiff = %d\n", len_tiff);
        name = malloc(len_tiff);

        if (name != NULL) {
                strncpy(name, path, len_cpy);
                name[len_cpy] = 0;

                strcat(name, ".tiff");
        }
        // printf("name = %s\n", name);

        return name;
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
                        ojpeg->mcu.v = row_per_strip;

                        compute_mcu(ojpeg, error);


                        uint32_t *mcu_RGB = NULL;

                        const uint32_t nb_pixels_max = ojpeg->mcu.size * ojpeg->mcu.nb;
                        ojpeg->raw_mcu = malloc(nb_pixels_max * sizeof(uint32_t));


                        if (ojpeg->raw_mcu == NULL)
                                *error = true;

                        else {
                                for (uint32_t i = 0; i < ojpeg->mcu.nb; i++) {

                                        mcu_RGB = &(ojpeg->raw_mcu[i * ojpeg->mcu.size]);

                                        read_tiff_file(file, mcu_RGB, ojpeg->mcu.h_dim, ojpeg->mcu.v_dim);
                                }
                        }

                        close_tiff_file(file);

                } else
                        *error = true;
        }
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
const uint8_t generic_qt[64] =
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

void detect_mcu(struct jpeg_data *jpeg, bool *error)
{
        uint8_t mcu_h = BLOCK_DIM;
        uint8_t mcu_v = BLOCK_DIM;

        /* Extract the MCU size */
        for (uint8_t i = 0; i < jpeg->nb_comps; i++) {
                mcu_h *= jpeg->comps[i].nb_blocks_h;
                mcu_v *= jpeg->comps[i].nb_blocks_v;
        }

        jpeg->mcu.h = mcu_h;
        jpeg->mcu.v = mcu_v;

        //printf("mcu_h = %u\n", mcu_h);
        //printf("mcu_v = %u\n", mcu_v);


        compute_mcu(jpeg, error);
}

void compute_mcu(struct jpeg_data *jpeg, bool *error)
{
        uint8_t mcu_h = jpeg->mcu.h;
        uint8_t mcu_v = jpeg->mcu.v;
        uint8_t mcu_h_dim = mcu_h / BLOCK_DIM;
        uint8_t mcu_v_dim = mcu_v / BLOCK_DIM;

        // printf("jpeg->width = %u\n", jpeg->width);
        // printf("jpeg->height = %u\n", jpeg->height);
        // printf("mcu_h = %u\n", mcu_h);
        // printf("mcu_v = %u\n", mcu_v);


        /* Various checks ensuring MCU validity */
        if (jpeg->width == 0
                || jpeg->height == 0
                || !(mcu_h == 8 || mcu_h == 16)
                || !(mcu_v == 8 || mcu_v == 16))
                *error = true;

        /* Count MCUs */
        uint32_t nb_mcu_h = mcu_per_dim(mcu_h, jpeg->width);
        uint32_t nb_mcu_v = mcu_per_dim(mcu_v, jpeg->height);
        uint32_t nb_mcu = nb_mcu_h * nb_mcu_v;


        /* Store computed MCU data */
        jpeg->mcu.h_dim = mcu_h_dim;
        jpeg->mcu.v_dim = mcu_v_dim;

        jpeg->mcu.nb_h = nb_mcu_h;
        jpeg->mcu.nb_v = nb_mcu_v;
        jpeg->mcu.nb = nb_mcu;

        jpeg->mcu.size = mcu_h * mcu_v;


        /* Y blocks */
        jpeg->comps[0].nb_blocks_h = mcu_h_dim;
        jpeg->comps[0].nb_blocks_v = mcu_v_dim;

        /* Cb blocks */
        jpeg->comps[1].nb_blocks_h = 1;
        jpeg->comps[1].nb_blocks_v = 1;

        /* Cr blocks */
        jpeg->comps[2].nb_blocks_h = 1;
        jpeg->comps[2].nb_blocks_v = 1;
}

/* Compresses raw mcu data, and computes Huffman / Quantification tables */
void compute_jpeg(struct jpeg_data *jpeg, bool *error)
{
        if (jpeg == NULL || *error) {
                *error = true;
                return;
        }

        // char *name = NULL;
        // struct tiff_file_desc *file = NULL;
        uint8_t i_c;
        uint8_t mcu_h = jpeg->mcu.h;
        uint8_t mcu_v = jpeg->mcu.v;
        uint8_t mcu_h_dim = jpeg->mcu.h_dim;
        uint8_t mcu_v_dim = jpeg->mcu.v_dim;


        uint32_t nb_mcu = jpeg->mcu.nb;

        // name = create_tiff_name(jpeg->path);

        //file = init_tiff_file(name, jpeg->width, jpeg->height, mcu_v);
        // file = init_tiff_file("out.tiff", jpeg->width, jpeg->height, mcu_v);


        uint8_t nb_blocks_h, nb_blocks_v, nb_blocks;
        uint8_t i_q;
        int32_t *last_DC;

        int32_t *block;
        int32_t iqzz[BLOCK_SIZE];
        uint8_t *upsampled;

        uint32_t *mcu_RGB = NULL;
        uint8_t data_YCbCr[3][mcu_h * mcu_v];
        uint8_t *mcu_YCbCr[3] = {
                (uint8_t*)&data_YCbCr[0],
                (uint8_t*)&data_YCbCr[1],
                (uint8_t*)&data_YCbCr[2]
        };


        /* Use 1 table for each tree (AC/DC)
         * Allocate 0x100 values because Huffman values use 8 bits */
        uint32_t freq_data[2][0x100];
        uint32_t *freqs[2] = {
                (uint32_t*)freq_data,
                (uint32_t*)&freq_data[1],
        };

        memset(freq_data, 0, sizeof(freq_data));


        const uint8_t nb_mcu_blocks = mcu_h_dim * mcu_v_dim + (jpeg->nb_comps - 1);
        // printf("Nb de blocks par MCU : %d\n", nb_mcu_blocks);

        jpeg->mcu_data = malloc(nb_mcu * nb_mcu_blocks * BLOCK_SIZE * sizeof(int32_t));

        if (jpeg->mcu_data == NULL) {
                *error = true;
                return;
        }


        uint32_t block_idx = 0;
        uint8_t idct[mcu_h_dim * mcu_v_dim][BLOCK_SIZE];

        // A commenter une fois le downsampler fonctionnel
        //memset(idct, 0, sizeof(idct));


        for (uint32_t i = 0; i < nb_mcu; i++) {

                mcu_RGB = &(jpeg->raw_mcu[i * jpeg->mcu.size]);


                if (jpeg->nb_comps == 3) {
                        ARGB_to_YCbCr(mcu_RGB, mcu_YCbCr, mcu_h_dim, mcu_v_dim);
                        // YCbCr_to_ARGB(mcu_YCbCr, mcu_RGB, mcu_h_dim, mcu_v_dim);
                        // ARGB_to_YCbCr(mcu_RGB, mcu_YCbCr, mcu_h_dim, mcu_v_dim);
                }

                else if (jpeg->nb_comps == 1) {
                        // printf("lol\n");
                        ARGB_to_Y(mcu_RGB, mcu_YCbCr[0], mcu_h_dim, mcu_v_dim);
                        // Y_to_ARGB(mcu_YCbCr[0], mcu_RGB, mcu_h_dim, mcu_v_dim);
                        // ARGB_to_Y(mcu_RGB, mcu_YCbCr[0], mcu_h_dim, mcu_v_dim);
                }
                else
                        *error = true;


                for (uint8_t j = 0; j < jpeg->nb_comps; j++) {


                        i_c = jpeg->comp_order[j];

                        nb_blocks_h = jpeg->comps[i_c].nb_blocks_h;
                        nb_blocks_v = jpeg->comps[i_c].nb_blocks_v;
                        nb_blocks = nb_blocks_h * nb_blocks_v;

                        i_q = jpeg->comps[i_c].i_q;

                        last_DC = &jpeg->comps[i_c].last_DC;


                        upsampled = mcu_YCbCr[i_c];
                        downsampler(upsampled, mcu_h_dim, mcu_v_dim, (uint8_t*)idct, nb_blocks_h, nb_blocks_v);

                        // printf("nb_blocks = %d\n", nb_blocks);
                        for (uint8_t n = 0; n < nb_blocks; n++) {

                                dct_block((uint8_t*)&idct[n], iqzz);
                                // idct_block(iqzz, (uint8_t*)&idct[n]);
                                // iqzz_block(block, iqzz, (uint8_t*)&jpeg->qtables[i_q]);

                                block = &jpeg->mcu_data[block_idx];


                                uint8_t *quantif = (uint8_t*)&jpeg->qtables[i_q];

                                qzz_block (iqzz, block, quantif);//, 3);

                                pack_block(NULL, NULL, last_DC, NULL, block, freqs);


                                // iqzz_block(block, iqzz, quantif);

                                // idct_block(iqzz, (uint8_t*)&idct[n]);
                                // test dct
                                // dct_block((uint8_t*)&idct[n], iqzz);
                                // idct_block(iqzz, (uint8_t*)&idct[n]);
                                block_idx += BLOCK_SIZE;
                        }
                }

                // write_tiff_file(file, mcu_RGB, mcu_h_dim, mcu_v_dim);
        }
        // printf("block_idx = %d\n", block_idx);
        // printf("expected = %d\n", nb_mcu * nb_mcu_blocks * BLOCK_SIZE);



        /*  Reset last_DC fields so that write_blocks can work fine */
        for (uint8_t i = 0; i < jpeg->nb_comps; i++)
                jpeg->comps[i].last_DC = 0;


        // DC table
        jpeg->htables[0][0] = create_huffman_tree(freqs[0], error);
        // printf("jpeg->htables[0][0] = %x\n", jpeg->htables[0][0]);

        // AC table
        jpeg->htables[1][0] = create_huffman_tree(freqs[1], error);
        // printf("jpeg->htables[1][0] = %x\n", jpeg->htables[1][0]);


        // close_tiff_file(file);
}

/* Writes previously compressed JPEG data */
void write_blocks(struct bitstream *stream, struct jpeg_data *jpeg, bool *error)
{
        if (stream == NULL || *error || jpeg == NULL || jpeg->state != ALL_OK) {
                *error = true;
                return;
        }

        uint8_t i_c;

        uint32_t nb_mcu = jpeg->mcu.nb;


        uint8_t nb_blocks_h, nb_blocks_v, nb_blocks;
        int32_t *last_DC;

        int32_t *block;

        uint32_t block_idx = 0;


        // file = init_tiff_file("out.tiff", jpeg->width, jpeg->height, mcu_v);



        for (uint32_t i = 0; i < nb_mcu; i++) {

                // printf("i = %d\n",i);
                for (uint8_t i = 0; i < jpeg->nb_comps; i++) {

                        i_c = jpeg->comp_order[i];

                        nb_blocks_h = jpeg->comps[i_c].nb_blocks_h;
                        nb_blocks_v = jpeg->comps[i_c].nb_blocks_v;
                        nb_blocks = nb_blocks_h * nb_blocks_v;

                        last_DC = &jpeg->comps[i_c].last_DC;
                        // printf("i_c = %d\n", i_c);
                        // printf("i_q = %d\n", i_q);


                        // printf("nb_blocks = %d\n", nb_blocks);
                        for (uint8_t n = 0; n < nb_blocks; n++) {


                                block = &jpeg->mcu_data[block_idx];



                                pack_block(stream, jpeg->htables[0][0], last_DC,
                                                     jpeg->htables[1][0], block, NULL);



                                block_idx += BLOCK_SIZE;
                        }

                }

                // write_tiff_file(file, mcu_RGB, mcu_h_dim, mcu_v_dim);
        }

        // close_tiff_file(file);

        flush_bitstream(stream);
}

void free_jpeg_data(struct jpeg_data *jpeg)
{
        if (jpeg == NULL)
                return;

        for (uint8_t i = 0; i < 2; i++)
                for (uint8_t j = 0; j < MAX_HTABLES; j++)
                        free_huffman_table(jpeg->htables[i][j]);
}

static inline uint16_t mcu_per_dim(uint8_t mcu, uint16_t dim)
{
        uint16_t nb = dim / mcu;

        return (dim % mcu) ? ++nb : nb;
}

void write_section(struct bitstream *stream, enum jpeg_section section,
                        struct jpeg_data *jpeg, bool *error)
{
        uint8_t byte;

        if (stream == NULL || error == NULL || *error)
                return;


        /* Write SECTION_HEAD */
        write_byte(stream, SECTION_HEAD);

        /* Write section marker */
        write_byte(stream, section);


        if (section == SOI || section == EOI)
                return;


        uint16_t size = 0;
        uint32_t pos_size;

        /* Write dummy size : rewrite later */
        pos_size = pos_bitstream(stream);
        write_short_BE(stream, 0);


        /* Process each section */
        switch (section) {

        case APP0:
                if (!*error) {
                        const char *jfif = "JFIF";

                        for (uint8_t i = 0; i < 5; i++)
                                write_byte(stream, jfif[i]);

                        /* Current JFIF version : 1.2 */
                        write_byte(stream, 0x01);
                        write_byte(stream, 0x02);

                        /* Density units : No units, aspect ratio only specified */
                        write_byte(stream, 0x00);

                        /* X density : Integer horizontal pixel density */
                        write_short_BE(stream, 1);

                        /* Y density : Integer vertical pixel density */
                        write_short_BE(stream, 1);


                        /* Thumbnail width (no thumbnail) */
                        write_byte(stream, 0);

                        /* Thumbnail height (no thumbnail) */
                        write_byte(stream, 0);
                }
                break;

        case COM:
                {
                        const char *comment = JPEG_COMMENT;
                        const uint16_t len = strlen(comment);

                        for (uint16_t i = 0; i < len; i++)
                                write_byte(stream, comment[i]);
                }

                break;

        case DQT:
                if (jpeg != NULL) {

                        bool mem[MAX_QTABLES];
                        memset(mem, 0, sizeof(mem));

                        /* Write all quantification tables */
                        for (uint8_t i = 0; i < jpeg->nb_comps; i++) {

                                /* Accuracy : 0 for 8 bits */
                                const uint8_t accuracy = 0;
                                uint8_t i_q = jpeg->comps[i].i_q;

                                if (mem[i_q])
                                        continue;

                                if (i_q >= MAX_QTABLES)
                                        *error = true;

                                else {
                                        mem[i_q] = true;
                                        byte = accuracy << 4;
                                        byte |= i_q & 0xF;
                                        // byte = i_q & 0xF;
                                        write_byte(stream, byte);
                                }


                                if (i_q < MAX_QTABLES) {
                                        uint8_t *qtable = (uint8_t*)&jpeg->qtables[i_q];

                                        for (uint8_t j = 0; j < BLOCK_SIZE; j++)
                                                write_byte(stream, qtable[j]);

                                } else
                                        *error = true;

                                jpeg->state |= DQT_OK;
                        }
                } else
                        *error = true;

                break;

        case SOF0:
                if (jpeg != NULL) {
                        const uint8_t accuracy = 8;
                        write_byte(stream, accuracy);


                        write_short_BE(stream, jpeg->height);
                        write_short_BE(stream, jpeg->width);

                        write_byte(stream, jpeg->nb_comps);


                        for (uint8_t i = 0; i < jpeg->nb_comps; i++) {
                                uint8_t i_c = i + 1;
                                uint8_t i_q = jpeg->comps[i].i_q;
                                uint8_t h_sampling_factor = jpeg->comps[i].nb_blocks_h;
                                uint8_t v_sampling_factor = jpeg->comps[i].nb_blocks_v;

                                write_byte(stream, i_c);


                                byte = h_sampling_factor << 4;
                                byte |= v_sampling_factor & 0xF;
                                write_byte(stream, byte);

                                write_byte(stream, i_q);

                                jpeg->state |= SOF0_OK;
                        }
                } else
                        *error = true;

                break;

        case DHT:
                if (jpeg != NULL) {
                        uint8_t i_h;
                        uint8_t type;

                        /* Write all Huffman tables */
                        for (uint8_t h = 0; h < 2; h++) {
                                type = h;

                                for (uint8_t i = 0; i < MAX_HTABLES; i++) {

                                        i_h = i;
                                        struct huff_table **table = &jpeg->htables[type][i_h];

                                        if (*table == NULL)
                                                continue;

                                        /*
                                         * 0 for DC
                                         * 1 for AC
                                         */
                                        byte = (type & 1) << 4;
                                        byte |= i_h & 0xF;
                                        write_byte(stream, byte);

                                        write_huffman_table(stream, table);
                                }
                        }

                        jpeg->state |= DHT_OK;
                } else
                        *error = true;

                break;

        /* Start Of Scan */
        case SOS:
                if (jpeg != NULL) {
                        uint8_t i_c;


                        write_byte(stream, jpeg->nb_comps);


                        for (uint8_t i = 0; i < jpeg->nb_comps; i++) {

                                // printf("i_c = %i\n", byte);
                                i_c = jpeg->comp_order[i];
                                write_byte(stream, i_c + 1);


                                uint8_t i_dc = jpeg->comps[i_c].i_dc;
                                uint8_t i_ac = jpeg->comps[i_c].i_ac;

                                byte = (i_dc << 4) | (i_ac & 0xF);

                                write_byte(stream, byte);
                        }

                        write_byte(stream, 0x00);
                        write_byte(stream, 0x3F);
                        write_byte(stream, 0x00);
                } else
                        *error = true;

                break;

        // case TEM:
        // case DNL:
        // case DHP:
        // case EXP:
        default:
                *error = true;
        }


        uint32_t end_section;

        end_section = pos_bitstream(stream);
        size = end_section - pos_size;

        /* Write section size and then restore current position */
        seek_bitstream(stream, pos_size);

        write_short_BE(stream, size);

        seek_bitstream(stream, end_section);
}

void write_header(struct bitstream *stream, struct jpeg_data *jpeg, bool *error)
{
        /* Write header data */
        write_section(stream, SOI, jpeg, error);
        write_section(stream, APP0, jpeg, error);
        write_section(stream, COM, jpeg, error);
        write_section(stream, SOF0, jpeg, error);

        /* Write all Quantification tables */
        write_section(stream, DQT, jpeg, error);

        /* Write all Huffman tables */
        write_section(stream, DHT, jpeg, error);

        /* Write SOS data */
        write_section(stream, SOS, jpeg, error);
}


