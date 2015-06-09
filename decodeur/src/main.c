
#include "common.h"
#include "bitstream.h"
#include "jpeg.h"
#include "huffman.h"
#include "library.h"


int main(int argc, char **argv)
{
        struct options options;

        /* Parse arguments and exit on failure */
        if (parse_args(argc, argv, &options))
                return EXIT_FAILURE;


        /* Open the input file */
        struct bitstream *stream = create_bitstream(options.input);

        if (stream != NULL) {

                bool error = false; 
                uint8_t marker;


                /* Initialize the jpeg structure with 0s */
                struct jpeg_data jpeg;
                memset(&jpeg, 0, sizeof(struct jpeg_data));

                /* Specify the output tiff path */
                jpeg.path = options.output;


                /* Read JPEG header data */
                read_header(stream, &jpeg, &error);

                /* Extract then write image data to tiff file */
                process_image(stream, &jpeg, &error);


                /* EOI check */
                if (!error) {
                        marker = read_section(stream, EOI, NULL, &error);

                        if (marker != EOI)
                                printf("ERROR : all JPEG files must end with an EOI section\n");

                        else
                                printf("JPEG file successfully decoded\n");
                } else
                        printf("ERROR : unsupported JPEG format\n");


                /* Close input JPEG file */
                free_bitstream(stream);

                /* Free any allocated JPEG data */
                free_jpeg_data(&jpeg);

        } else
                printf("ERROR : Invalid input JPEG path\n");


        SAFE_FREE(options.output);


        return EXIT_SUCCESS;
}


