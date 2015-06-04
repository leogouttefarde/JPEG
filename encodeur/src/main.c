
#include "common.h"
#include "bitstream.h"
#include "jpeg.h"
#include "library.h"


int main(int argc, char **argv)
{
        if (argc < 2) {
                printf(USAGE, argv[0]);
                return EXIT_FAILURE;
        }


        char *path = argv[1];

        if (!is_valid_jpeg(path)
                // && !is_valid_tiff(path)
                ) {
                printf("ERROR : Invalid file extension, .tiff .tif .jpg or .jpeg expected\n");
                return EXIT_FAILURE;
        }

        char *dest = "out.jpg";
        struct bitstream *stream = create_bitstream(dest, WRONLY);

        if (stream != NULL) {
                bool error = false;


                struct jpeg_data jpeg;
                memset(&jpeg, 0, sizeof(jpeg));

                jpeg.path = path;

                // Fake MCU definition (could be specified as option for tiff,
                // and later used for jpeg reencoding too)
                // Currently useless, reusing previous JPEG detected MCU sizes
                jpeg.mcu.h = 8;
                jpeg.mcu.v = 8;


                /* Read input image */
                read_image(&jpeg, &error);


                /* Compute Huffman and Quantification tables */
                compute_jpeg(&jpeg, &error);

                /* Free raw image data */
                SAFE_FREE(jpeg.raw_mcu);


                /* Write JPEG header */
                write_header(stream, &jpeg, &error);

                /* Write computed JPEG data */
                write_blocks(stream, &jpeg, &error);

                /* End JPEG file */
                write_section(stream, EOI, NULL, &error);


                if (error)
                        printf("ERROR : unsupported JPEG format\n");
                else
                        printf("JPEG file successfully encoded\n");


                free_bitstream(stream);

                /* Free compressed JPEG data */
                SAFE_FREE(jpeg.mcu_data);

                /* Free JPEG Huffman trees */
                free_jpeg_data(&jpeg);
        }


        return EXIT_SUCCESS;
}


