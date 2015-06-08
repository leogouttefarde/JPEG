
#include "jpeg.h"
#include "huffman.h"
#include "unpack.h"
#include "iqzz.h"
#include "idct.h"
#include "tiff.h"
#include "conv.h"
#include "upsampler.h"
#include "library.h"


static inline uint16_t mcu_per_dim(uint8_t mcu, uint16_t dim);

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
                                        uint8_t i_q = byte & 0xF;

                                        if (i_q < MAX_QTABLES) {
                                                uint8_t *qtable = (uint8_t*)&jpeg->qtables[i_q];

                                                for (uint8_t i = 0; i < BLOCK_SIZE; i++) {
                                                        *error |= read_byte(stream, &qtable[i]);
                                                        unread--;
                                                }
                                        } else
                                                *error = true;

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

                                if (nb_byte_read == (uint16_t)-1 || table == NULL)
                                        *error = true;


                                if (!*error)
                                        jpeg->htables[type][i_h] = table;

                                else if (table != NULL)
                                        free_huffman_table(table);


                                unread -= nb_byte_read;
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

/* Extract then write image data to tiff file */
void process_image(struct bitstream *stream, struct jpeg_data *jpeg, bool *error)
{
        if (stream == NULL || *error || jpeg == NULL || jpeg->state != ALL_OK) {
                *error = true;
                return;
        }

        struct tiff_file_desc *file = NULL;
        uint8_t i_c;
        uint8_t mcu_h = BLOCK_DIM;
        uint8_t mcu_v = BLOCK_DIM;
        uint8_t mcu_h_dim = 1;
        uint8_t mcu_v_dim = 1;

        /* Extract the MCU size */
        for (uint8_t i = 0; i < jpeg->nb_comps; i++) {
                i_c = jpeg->comp_order[i];

                mcu_h_dim *= jpeg->comps[i_c].nb_blocks_h;
                mcu_v_dim *= jpeg->comps[i_c].nb_blocks_v;
        }

        mcu_h *= mcu_h_dim;
        mcu_v *= mcu_v_dim;


        uint32_t nb_mcu_h = mcu_per_dim(mcu_h, jpeg->width);
        uint32_t nb_mcu_v = mcu_per_dim(mcu_v, jpeg->height);
        uint32_t nb_mcu = nb_mcu_h * nb_mcu_v;

        file = init_tiff_file(jpeg->path, jpeg->width, jpeg->height, mcu_v);

        if (file != NULL) {
                uint8_t nb_blocks_h, nb_blocks_v, nb_blocks;
                uint8_t i_dc, i_ac, i_q;
                int32_t *last_DC;

                int32_t block[BLOCK_SIZE];
                int32_t iqzz[BLOCK_SIZE];
                uint8_t *upsampled;

                uint32_t mcu_RGB[mcu_h * mcu_v];
                uint8_t data_YCbCr[3][mcu_h * mcu_v];
                uint8_t *mcu_YCbCr[3] = {
                        (uint8_t*)&data_YCbCr[0],
                        (uint8_t*)&data_YCbCr[1],
                        (uint8_t*)&data_YCbCr[2]
                };

                uint8_t idct[mcu_h_dim * mcu_v_dim][BLOCK_SIZE];


                for (uint32_t i = 0; i < nb_mcu; i++) {
                        for (uint8_t j = 0; j < jpeg->nb_comps; j++) {

                                i_c = jpeg->comp_order[j];
                                i_q = jpeg->comps[i_c].i_q;
                                i_dc = jpeg->comps[i_c].i_dc;
                                i_ac = jpeg->comps[i_c].i_ac;
                                nb_blocks_h = jpeg->comps[i_c].nb_blocks_h;
                                nb_blocks_v = jpeg->comps[i_c].nb_blocks_v;
                                nb_blocks = nb_blocks_h * nb_blocks_v;
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

                        if (jpeg->nb_comps == 3)
                                YCbCr_to_ARGB(mcu_YCbCr, mcu_RGB, mcu_h_dim, mcu_v_dim);

                        else if (jpeg->nb_comps == 1)
                                Y_to_ARGB(mcu_YCbCr[0], mcu_RGB, mcu_h_dim, mcu_v_dim);
                        else
                                *error = true;

                        write_tiff_file(file, mcu_RGB, mcu_h_dim, mcu_v_dim);
                }

                close_tiff_file(file);

        } else
                *error = true;


        skip_bitstream_until(stream, SECTION_HEAD);
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


