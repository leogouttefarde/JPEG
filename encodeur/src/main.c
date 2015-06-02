
#include "common.h"
#include "bitstream.h"
#include "jpeg.h"
#include "library.h"


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


        struct bitstream *stream = create_bitstream(path, RDONLY);
        struct bitstream *ostream = create_bitstream("out.jpg", WRONLY);

        if (stream != NULL && ostream) {

                bool error = false;
                // uint8_t marker;

                struct jpeg_data jpeg;
                memset(&jpeg, 0, sizeof(struct jpeg_data));

                jpeg.path = path;



                /* Read header data */
                read_header(stream, &jpeg, &error);


                struct jpeg_data ojpeg;
                memset(&ojpeg, 0, sizeof(struct jpeg_data));


                ojpeg.height = jpeg.height;
                ojpeg.width = jpeg.width;

                ojpeg.nb_comps = jpeg.nb_comps;

                memcpy(&ojpeg.comps, &jpeg.comps, sizeof(jpeg.comps));

                for (uint8_t i = 0; i < ojpeg.nb_comps; i++) {
                        // ojpeg.comps[i] = jpeg.comps[i];

                        ojpeg.comps[i].i_dc = 0;
                        ojpeg.comps[i].i_ac = 0;

                        // /* SOF0 data */
                        // uint8_t nb_blocks_h;
                        // uint8_t nb_blocks_v;
                        // uint8_t i_q;

                        // /* SOS data */
                        // uint8_t i_dc;
                        // uint8_t i_ac;

                        // /* Last DC decoded value */
                        // int32_t last_DC;
                }


                for (uint8_t i = 0; i < ojpeg.nb_comps; i++)
                        ojpeg.comp_order[i] = jpeg.comp_order[i];



                //memcpy(&ojpeg.qtables, &jpeg.qtables, sizeof(jpeg.qtables));
                memset(&ojpeg.htables, 0, sizeof(ojpeg.htables));


                // printf("pos stream\n");
                uint32_t pos = pos_bitstream(stream);

                /* Compute raw Huffman tables */
                process_image(stream, NULL, &jpeg, &ojpeg, &error);


                write_header(ostream, &ojpeg, &error);

                //huffman_export("dc_tree.dot", ojpeg.htables[0][0]);
                //huffman_export("ac_tree.dot", ojpeg.htables[1][0]);


                seek_bitstream(stream, pos);

                // printf("pos stream\n");
                // uint32_t pos2 = pos_bitstream(stream);


                for (uint8_t i = 0; i < jpeg.nb_comps; i++)
                        jpeg.comps[i].last_DC = 0;

                for (uint8_t i = 0; i < ojpeg.nb_comps; i++)
                        ojpeg.comps[i].last_DC = 0;



                /* Write new JPEG data */
                process_image(stream, ostream, &jpeg, &ojpeg, &error);

                write_section(ostream, EOI, NULL, &error);


                /* EOI check */
                if (!error) {
                        // marker = read_section(stream, EOI, NULL, &error);

                        // if (marker != EOI)
                        //         printf("ERROR : all JPEG files must end with an EOI section\n");
                        // else
                                printf("JPEG file successfully encoded\n");
                } else
                        printf("ERROR : unsupported JPEG format\n");

                free_bitstream(ostream);

                /* Close input JPEG file */
                free_bitstream(stream);

                /* Free any allocated JPEG data */
                free_jpeg_data(&jpeg);
                free_jpeg_data(&ojpeg);
        }


        return EXIT_SUCCESS;
}


