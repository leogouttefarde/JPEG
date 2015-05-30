
#ifndef __JPEG_H__
#define __JPEG_H__

#include "common.h"

#define MAX_COLORS 3
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
        ANY  = 0
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

struct jpeg_data {
        char *path;

        uint16_t height, width;

        /* Number of color components */
        uint8_t nb_comps;

        /* Color components */
        struct comp comps[MAX_COLORS];

        /* Color index order */
        uint8_t comp_order[MAX_COLORS];

        /* Huffman tables */
        struct huff_table *htables[2][MAX_HTABLES];

        /* Quantification tables */
        uint8_t qtables[MAX_COLORS][BLOCK_SIZE];
};



/* Read a section */
uint8_t read_section(struct bitstream *stream, enum jpeg_section section,
                        struct jpeg_data *jpeg, bool *error);

/* Read header data */
void read_header(struct bitstream *stream, struct jpeg_data *jpeg, bool *error);

/* Extract then write image data to tiff file */
void process_image(struct bitstream *stream, struct jpeg_data *jpeg, bool *error);


void free_jpeg_data(struct jpeg_data *jpeg);



#endif