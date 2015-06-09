
#ifndef __ENCODE_H__
#define __ENCODE_H__

#include "common.h"
#include "bitstream.h"
#include "decode.h"


/*
 * Command line options
 */
struct options {

        /* Indicates if we must encode or else decode */
        bool encode;

        /* Input file path */
        char *input;

        /* Output file path */
        char *output;

        /* Required MCU dimensions */
        uint8_t mcu_h;
        uint8_t mcu_v;

        /* Compression rate */
        uint8_t compression;

        /*
         * Indicates if we must
         * produce a grayscale image
         */
        bool gray;
};

/* Compiling definitions */
struct jpeg_data;
enum jpeg_section;


/* Frees all JPEG Huffman tables */
extern void free_jpeg_data(struct jpeg_data *jpeg);

/* Writes a specific JPEG section */
extern void write_section(struct bitstream *stream, enum jpeg_section section,
                          struct jpeg_data *jpeg, bool *error);

/* Writes a whole JPEG header */
extern void write_header(struct bitstream *stream, struct jpeg_data *jpeg, bool *error);

/* Compresses raw mcu data, and computes Huffman / Quantification tables */
extern void compute_jpeg(struct jpeg_data *jpeg, bool *error);

/* Writes previously compressed JPEG data */
extern void write_blocks(struct bitstream *stream, struct jpeg_data *jpeg, bool *error);

/* Detects MCU informations from header data */
extern void detect_mcu(struct jpeg_data *jpeg, bool *error);

/* Computes MCU informations */
extern void compute_mcu(struct jpeg_data *jpeg, bool *error);


#endif
