
#include "tiff.h"
#include "decode.h"
#include "library.h"

#define INPUT  "tests/tiff/ice_age.tiff"
#define OUTPUT "tests/tiff/ice_age_test_tiff.tiff"


int main()
{
        struct tiff_file_desc *in = NULL;
        uint32_t width, height;

        /* Read TIFF header */
        in = init_tiff_read(INPUT, &width, &height);

        if (in == NULL) {
                printf("ERROR : invalid input TIFF file\n");
                return EXIT_FAILURE;
        }


        /* Read all raw RGB data from the TIFF file */

        uint32_t *line = NULL;

        const uint32_t nb_pixels_max = width * height;
        uint32_t *raw_data = malloc(nb_pixels_max * sizeof(uint32_t));

        if (raw_data != NULL){

                /* Read all RGB lines */
                for (uint32_t i = 0; i < height; i++) {

                        /* Read one RGB line */
                        line = &(raw_data[i * width]);
                        read_tiff_line(in, line);
                }
        }

        close_tiff_file(in);


        struct tiff_file_desc *out = NULL;

        struct mcu_info mcu;

        mcu.h = 16;
        mcu.v = 8;

        out = init_tiff_file(OUTPUT, width, height, mcu.v);

        if (out != NULL) {

                mcu.h_dim = mcu.h/8;
                mcu.v_dim = mcu.v/8;
                mcu.nb_h = width / mcu.h + width % mcu.h;
                mcu.nb_v = height / mcu.v + height % mcu.v;
                mcu.nb = mcu.nb_h * mcu.nb_v;
                mcu.size = mcu.h * mcu.v;


                uint32_t *mcu_RGB;
                uint32_t *mcu_data = image_to_mcu(raw_data, &mcu, width, height);

                for (uint32_t i = 0; i < mcu.nb; i++) {
                        mcu_RGB = &(mcu_data[i * mcu.size]);
                        write_tiff_file(out, mcu_RGB, mcu.h_dim,
                                        mcu.v_dim);
                }

                close_tiff_file(out);

                printf("TIFF successfully reencoded as %s\n", OUTPUT);

        } else
                printf("ERROR : invalid output TIFF file\n");


        return EXIT_SUCCESS;
}
