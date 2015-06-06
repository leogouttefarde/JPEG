
#include "common.h"
#include "bitstream.h"
#include "encode.h"
#include "decode.h"
#include "library.h"


int main(int argc, char **argv)
{
        bool error = false;
        struct options options;

        if (parse_args(argc, argv, &options))
                return EXIT_FAILURE;


        int ret = EXIT_SUCCESS;
        struct bitstream *stream = create_bitstream(options.output, WRONLY);

        if (stream != NULL) {
                struct jpeg_data jpeg;
                memset(&jpeg, 0, sizeof(jpeg));

                jpeg.path = options.input;

                // Only used for tiff encoding
                jpeg.mcu.h = options.mcu_h;
                jpeg.mcu.v = options.mcu_v;

                jpeg.compression = options.compression;


                /* Read input image */
                read_image(&jpeg, &error);


                if (options.gray) {
                        jpeg.nb_comps = 1;

                        options.mcu_h = BLOCK_DIM;
                        options.mcu_v = BLOCK_DIM;
                }


                if (jpeg.mcu.h != options.mcu_h
                        || jpeg.mcu.v != options.mcu_v
                        || jpeg.is_plain_image) {

                        uint32_t *image = jpeg.raw_data;

                        if (!jpeg.is_plain_image) {
                                image = mcu_to_image(jpeg.raw_data,
                                                        &jpeg.mcu,
                                                        jpeg.width,
                                                        jpeg.height);

                                SAFE_FREE(jpeg.raw_data);
                        }


                        jpeg.mcu.h = options.mcu_h;
                        jpeg.mcu.v = options.mcu_v;

                        compute_mcu(&jpeg, &error);


                        uint32_t *data = image_to_mcu(image,
                                                        &jpeg.mcu,
                                                        jpeg.width,
                                                        jpeg.height);

                        SAFE_FREE(image);

                        jpeg.raw_data = data;
                }


                /* Compute Huffman and Quantification tables */
                compute_jpeg(&jpeg, &error);

                /* Free raw image data */
                SAFE_FREE(jpeg.raw_data);


                /* Write JPEG header */
                write_header(stream, &jpeg, &error);

                /* Write computed JPEG data */
                write_blocks(stream, &jpeg, &error);

                /* End JPEG file */
                write_section(stream, EOI, NULL, &error);


                free_bitstream(stream);


                if (error) {
                        printf("JPEG compression failed\n");
                        ret = EXIT_FAILURE;

                        /* Remove the invalid created file */
                        remove(options.output);
                }

                else
                        printf("JPEG successfully encoded\n");


                /* Free compressed JPEG data */
                SAFE_FREE(jpeg.mcu_data);

                /* Free JPEG Huffman trees */
                free_jpeg_data(&jpeg);
        }


        return ret;
}

