
#include "common.h"
#include "bitstream.h"
#include "jpeg.h"
#include "library.h"
#include <unistd.h>
#include <getopt.h>

int main(int argc, char **argv)
{
        uint8_t quality = 2;
        char *path = NULL;
        char *dest = NULL;

        int c;
        char *copt = NULL;

        while ( (c = getopt(argc, argv, "o:c:")) != -1) {

                // Pour détecter arguments doubles
                // int this_option_optind = optind ? optind : 1;

                switch (c) {
                        case 'o':
                                dest = optarg;
                                // printf ("Fichier de sortie : '%s'\n", optarg);
                                break;

                        case 'c':
                                copt = optarg;
                                // printf ("Compression [0-25] : '%s'\n", optarg);
                                break;

                        default:
                                printf ("Unrecognized option : %c\n", c);
                }
        }


        if (copt != NULL) {
                char *endptr = NULL;
                int32_t val = strtol(copt, &endptr, 10);

                if (endptr != copt) {
                        if (0 <= val && val <= 25) {
                                quality = val;
                                // printf("Compression : %d\n", quality);
                        }
                }
        }



        if (optind < argc) {

                // printf("optind = %u\n", optind);
                path = argv[optind];
                // printf("path = %s\n", path);
                optind++;

                if (optind < argc) {
                        printf ("Paramètres non reconnus : ");
                        while (optind < argc)
                                printf ("%s ", argv[optind++]);

                        printf ("\n");
                }
        }


        if (path == NULL || dest == NULL) {
                printf(USAGE, argv[0]);
                return EXIT_FAILURE;
        }



        if (!is_valid_jpeg(path)
                && !is_valid_tiff(path)
                ) {
                printf("ERROR : Invalid file extension, .tiff .tif .jpg or .jpeg expected\n");
                return EXIT_FAILURE;
        }

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
                        printf("ERROR : unsupported input format\n");

                else
                        printf("JPEG file successfully created\n");


                free_bitstream(stream);

                /* Free compressed JPEG data */
                SAFE_FREE(jpeg.mcu_data);

                /* Free JPEG Huffman trees */
                free_jpeg_data(&jpeg);
        }


        return EXIT_SUCCESS;
}


