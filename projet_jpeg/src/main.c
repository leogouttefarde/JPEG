
#include "common.h"
#include "bitstream.h"
#include "jpeg.h"


int main(int argc, char **argv)
{
        if (argc < 2) {
                printf("Usage : %s <jpeg_file>\n", argv[0]);
                return EXIT_FAILURE;
        }


        char *path = argv[1];

        if (!is_valid_ext(path)) {
                printf("ERROR : Invalid file extension, .jpg or .jpeg expected\n");
                return EXIT_FAILURE;
        }


        struct bitstream *stream = create_bitstream(path);

        if (stream != NULL) {

                bool error = false;
                uint8_t marker;

                struct jpeg_data jpeg;
                memset(&jpeg, 0, sizeof(struct jpeg_data));

                jpeg.path = path;


                /* Read header data */
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
        }


        return EXIT_SUCCESS;
}


