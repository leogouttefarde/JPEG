#ifndef __TIFF_H__
#define __TIFF_H__

#include "common.h"


/* Structure permettant de stocker les informations nécessaire à
 * la lecture ou l'écriture des données de l'image dans un fichier TIFF. */
struct tiff_file_desc;


/*
 * Initialisation de la lecture d'un fichier TIFF, avec les sorties suivantes :
 *  - width : la largeur de l'image lue
 *  - height: la hauteur de l'image lue
 */
extern struct tiff_file_desc *init_tiff_read(const char *path, uint32_t *width, uint32_t *height);

/* Ferme le fichier associé à la structure tiff_file_desc passée en
 * paramètre et désalloue la mémoire occupée par cette structure. */
extern void close_tiff_file(struct tiff_file_desc *tfd);

/* Lit une ligne de l'image TIFF ouverte avec init_tiff_read.
 * Renvoie true si une erreur est survenue, false si pas d'erreur. */
extern bool read_tiff_line(struct tiff_file_desc *tfd, uint32_t *line_rgb);


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
extern void write_tiff_file (struct tiff_file_desc *tfd,
                                uint32_t *mcu_rgb,
                                uint8_t nb_blocks_h,
                                uint8_t nb_blocks_v) ;


#endif
