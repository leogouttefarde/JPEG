#include "tiff.h"

void affiche_mcu(uint32_t id,uint32_t *mcu_rgb, uint8_t nb_blocks_h, uint8_t nb_blocks_v){
	printf("MCU ID: %u \n",id);
	for (uint32_t i=0; i < nb_blocks_v*8;i++ ){
		for (uint32_t j=0; j < nb_blocks_h*8;j++ ){
			printf("%#16x  ",mcu_rgb[i*nb_blocks_h*8+j]);
		}
		printf("\n");
	}
	printf("==================================================\n");
	
}
int main(){

	struct tiff_file_desc *tfd_read = init_tiff_file_read ("tests/ice_age_version_profs.tiff");
	struct tiff_file_desc *tfd_write = init_tiff_file ("tests/ice_age_version_profs_written.tiff", tfd_read -> width, tfd_read -> height, tfd_read -> rows_per_strip);
	
	uint8_t nb_blocks_h = 1;
	uint8_t nb_blocks_v = 1;
	
	uint32_t *mcu_rgb = malloc(64*nb_blocks_h*nb_blocks_v*sizeof(uint32_t));

	/*read_tiff_file (tfd_read,mcu_rgb,
			  nb_blocks_h,
			  nb_blocks_v);*/
	for (uint32_t i=0; i<(((tfd_read -> width/8))*((tfd_read -> height/8)+1)); i++){
	
		read_tiff_file (tfd_read,mcu_rgb,
			  nb_blocks_h,
			  nb_blocks_v);
	
		affiche_mcu(i,mcu_rgb, nb_blocks_h, nb_blocks_v);
		write_tiff_file (tfd_write,mcu_rgb,
			  nb_blocks_h,
			  nb_blocks_v);
	
	}
	
	return 0;
}
