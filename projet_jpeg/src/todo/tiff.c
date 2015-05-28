
#include "tiff.h"


/* Initialisation du fichier TIFF résultat, avec les paramètres suivants:
   - width: la largeur de l'image ;
   - height: la hauteur de l'image ;
   - row_per_strip: le nombre de lignes de pixels par bande.
 */
struct tiff_file_desc *init_tiff_file (const char *file_name,
                                              uint32_t width,
                                              uint32_t height,
                                              uint32_t row_per_strip)
{
        return NULL;
}

/* Ferme le fichier associé à la structure tiff_file_desc passée en
 * paramètre et désalloue la mémoire occupée par cette structure. */
int32_t close_tiff_file(struct tiff_file_desc *tfd)
{
        return 0;
}

/* Ecrit le contenu de la MCU passée en paramètre dans le fichier TIFF
 * représenté par la structure tiff_file_desc tfd. nb_blocks_h et
 * nb_blocks_v représentent les nombres de blocs 8x8 composant la MCU
 * en horizontal et en vertical. */
int32_t write_tiff_file (struct tiff_file_desc *tfd,
                                uint32_t *mcu_rgb,
                                uint8_t nb_blocks_h,
                                uint8_t nb_blocks_v)
{
        return 0;
}

