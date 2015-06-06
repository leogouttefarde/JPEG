
#ifndef __DECODE_H__
#define __DECODE_H__

#include "common.h"
#include "bitstream.h"
#include "encode.h"

#define MAX_COMPS 3
#define MAX_HTABLES 4
#define MAX_QTABLES 0x10
#define SECTION_HEAD 0xFF


/* JPEG section markers */
enum jpeg_section {
        SOI  = 0xD8,
        APP0 = 0xE0,
        COM  = 0xFE,
        DQT  = 0xDB,
        SOF0 = 0xC0,
        DHT  = 0xC4,
        SOS  = 0xDA,
        EOI  = 0xD9,
        ANY  = 0,
        TEM  = 0x01,
        DNL  = 0xDC,
        DHP  = 0xDE,
        EXP  = 0xDF,
        DRI  = 0xDD
};

/* JPEG status check */
enum jpeg_status {
        DHT_OK  = 1,
        DQT_OK  = 2,
        SOF0_OK = 4,
        ALL_OK = 7
};


struct comp {

        /* SOF0 data */
        uint8_t nb_blocks_h;
        uint8_t nb_blocks_v;
        uint8_t i_q;

        /* SOS data */
        uint8_t i_dc;
        uint8_t i_ac;

        /* Last DC decoded value */
        int32_t last_DC;
};

struct mcu_info {
        uint32_t h;
        uint32_t v;
        uint8_t h_dim;
        uint8_t v_dim;
        uint32_t nb_h;
        uint32_t nb_v;
        uint32_t nb;
        uint32_t size;
};

struct jpeg_data {

        char *path;

        uint16_t height, width;

        /* Number of color components */
        uint8_t nb_comps;

        /* Color components */
        struct comp comps[MAX_COMPS];

        /* Color index order */
        uint8_t comp_order[MAX_COMPS];

        /* Huffman tables */
        struct huff_table *htables[2][MAX_HTABLES];

        /* Quantification tables */
        uint8_t qtables[MAX_QTABLES][BLOCK_SIZE];

        uint8_t compression;

        uint8_t state;

        uint32_t *raw_mcu;

        struct mcu_info mcu;

        int32_t *mcu_data;
};


/* Read a section */
extern uint8_t read_section(struct bitstream *stream, enum jpeg_section section,
                        struct jpeg_data *jpeg, bool *error);

/* Read header data */
extern void read_header(struct bitstream *stream, struct jpeg_data *jpeg, bool *error);

/* Extract raw image data */
extern void read_image(struct jpeg_data *jpeg, bool *error);


#endif
