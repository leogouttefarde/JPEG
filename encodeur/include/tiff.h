#ifndef __TIFF_H__
#define __TIFF_H__


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"

/* Structure permettant de stocker les informations nécessaire à
 * la lecture ou l'écriture des données de l'image dans un fichier TIFF. */
//struct tiff_file_desc;
struct tiff_file_desc {
	FILE *file;
	bool is_le; // Vaut true si le fichier est en little endian
	uint32_t ptr_ifd;
	uint32_t ifd_count;
	uint32_t width;
	uint32_t height;
	uint16_t bits_per_sample;
	uint16_t compression;
	uint16_t photometric_interpretation;
	uint32_t strip_offsets_count;
	uint32_t *strip_offsets;
	uint16_t samples_per_pixels;
	uint32_t rows_per_strip;
	uint32_t strip_bytes_count;
	uint32_t *strip_bytes;
	uint32_t x_resolution_num;
	uint32_t x_resolution_denum;
	uint32_t y_resolution_num;
	uint32_t y_resolution_denum;
	uint16_t resolution_unit;

	
	uint32_t current_line;
	uint32_t next_pos_mcu;
	uint32_t size_line;
	uint32_t row_size;
	uint32_t nb_strips;

};

/* Renvoie un pointeur vers le tiff_file_desc correspondant au fichier tiff de * path file_name après la lecture du header
 */
extern struct tiff_file_desc *init_tiff_file_read (const char *file_name);

/* Ferme le fichier associé à la structure tiff_file_desc passée en
 * paramètre et désalloue la mémoire occupée par cette structure. */
extern void close_tiff_file(struct tiff_file_desc *tfd);

/* Lit une MCU composée de nb_blocks_h et nb_blocks_v 
 * blocs 8x8  en horizontal et en vertical à partir du 
 * fichier fichier TIFF représenté par la structure 
 * tiff_file_desc tfd. */
extern void read_tiff_file (struct tiff_file_desc *tfd, uint32_t *mcu_rgb,
			  uint8_t nb_blocks_h,
			    uint8_t nb_blocks_v);

/* Initialisation du fichier TIFF résultat, avec les paramètres suivants:
   - width: la largeur de l'image ;
   - height: la hauteur de l'image ;
   - row_per_strip: le nombre de lignes de pixels par bande.
 */
extern struct tiff_file_desc *init_tiff_file (const char *file_name,
                                              uint32_t width,
                                              uint32_t height,
                                              uint32_t row_per_strip);

/* Ecrit le contenu de la MCU passée en paramètre dans le fichier TIFF
 * représenté par la structure tiff_file_desc tfd. nb_blocks_h et
 * nb_blocks_v représentent les nombres de blocs 8x8 composant la MCU
 * en horizontal et en vertical. */
extern void write_tiff_file (struct tiff_file_desc *tfd,  uint32_t *mcu_rgb,
			     uint8_t nb_blocks_h,uint8_t nb_blocks_v) ;

#endif
