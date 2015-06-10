
#ifndef __JPEG_H__
#define __JPEG_H__

#include "common.h"
#include "bitstream.h"

#define MAX_COMPS 3
#define MAX_HTABLES 4

/*
 * Quantification index on 4 bits :
 * => max 16 quantification tables
 */
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
        EXP  = 0xDF
};

/* JPEG status check */
enum jpeg_status {
        DHT_OK  = 1,
        DQT_OK  = 2,
        SOF0_OK = 4,
        ALL_OK  = 7
};

/* Command line options */
struct options {
        char *input;
        char *output;
};

/* Component informations */
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

/* JPEG informations */
struct jpeg_data {

        /* File path */
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
        
        /* JPEG status check */
        uint8_t state;
};



/* Read a jpeg section */
extern uint8_t read_section(struct bitstream *stream, enum jpeg_section section,
                        struct jpeg_data *jpeg, bool *error);

/* Read a whole JPEG header */
extern void read_header(struct bitstream *stream, struct jpeg_data *jpeg, bool *error);

/* Extract, decode jpeg data and write image data to tiff file */
extern void process_image(struct bitstream *stream, struct jpeg_data *jpeg, bool *error);

/* Free jpeg_data structure */
extern void free_jpeg_data(struct jpeg_data *jpeg);



#endif
