#include "tiff.h"
#include "decode.h"
#include "library.h"

int main(){
	
        struct tiff_file_desc *in = NULL;
        uint32_t width, height;
                
        /* Read TIFF header */
        in = init_tiff_read("tests/ice_age_version_profs.tiff", &width, &height);

        if (in != NULL) {

                

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
                uint32_t rows_per_strip = 16;

                out = init_tiff_file("tests/ice_age_version_profs_test_tiff.tiff", width,
                                     height, rows_per_strip);

                if (out != NULL) {

                        uint32_t *mcu_RGB;
                       
                        struct mcu_info mcu;
                        
                        mcu.h = 16;
                        mcu.v = 8;
                        mcu.h_dim = mcu.h/8;
                        mcu.v_dim = mcu.v/8;
                        mcu.nb_h = width/mcu.h+(width%mcu.h==0?0:1);
                        mcu.nb_v = height/mcu.v+(height%mcu.v==0?0:1);
                        mcu.nb = mcu.nb_h*mcu.nb_v;
                        mcu.size = mcu.h*mcu.v;
                                
        
                       
                        uint32_t *mcu_data = image_to_mcu(raw_data, &mcu, width, height);
                        
                        for (uint32_t i = 0; i < mcu.nb; i++) {
                                mcu_RGB = &(mcu_data[i * mcu.size]);
                                write_tiff_file(out, mcu_RGB, mcu.h_dim,
                                                mcu.v_dim);
                        }

                        close_tiff_file(out);

                } 

                

        } else {
                printf("ERROR : invalid input TIFF file\n");
        }

        
        
	

	return 0;
}
