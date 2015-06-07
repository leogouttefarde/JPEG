
#ifndef __ENCODE_H__
#define __ENCODE_H__

#include "common.h"
#include "bitstream.h"
#include "decode.h"


struct options {

        char *input;
        char *output;

        uint8_t mcu_h;
        uint8_t mcu_v;

        uint8_t compression;

        bool gray;
};

struct jpeg_data;
enum jpeg_section;


extern void free_jpeg_data(struct jpeg_data *jpeg);

extern void write_section(struct bitstream *stream, enum jpeg_section section,
                        struct jpeg_data *jpeg, bool *error);

extern void write_header(struct bitstream *stream, struct jpeg_data *jpeg, bool *error);

/* Compresses raw mcu data, and computes Huffman / Quantification tables */
extern void compute_jpeg(struct jpeg_data *jpeg, bool *error);

/* Writes previously compressed JPEG data */
extern void write_blocks(struct bitstream *stream, struct jpeg_data *jpeg, bool *error);


extern void detect_mcu(struct jpeg_data *jpeg, bool *error);

extern void compute_mcu(struct jpeg_data *jpeg, bool *error);


#endif
