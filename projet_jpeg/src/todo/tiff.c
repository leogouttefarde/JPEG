
#include "tiff.h"


// Macros pour l'écriture dans le fichier
#define SHORT 0x0003
#define LONG 0x0004
#define RATI 0x0005

#define ZERO 0x0000;
#define UN 0x0001;
#define DEUX 0x0002;
#define TROIS 0x0003;


struct tiff_file_desc {
	FILE *file;
	uint32_t width;
	uint32_t height;
	uint32_t rows_per_strip;
	uint32_t nb_strips;
};

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
        struct tiff_file_desc *tfd = malloc(sizeof(struct tiff_file_desc));
	
	tfd -> file = fopen(file_name,"wb");
	if (tfd -> file == NULL){
		perror("Erreur: ");
		exit(1);
	}
	
	tfd -> width = width;
	tfd -> height = height;
	tfd -> rows_per_strip = row_per_strip;
	
	tfd -> nb_strips = tfd -> height / tfd -> rows_per_strip;
	tfd-> nb_strips = ((tfd -> nb_strips * tfd -> rows_per_strip) < tfd -> height ? tfd -> nb_strips + 1 : tfd -> nb_strips);

	// écriture du header dans le fichier
	//===================================
	uint32_t taille_buffer = (90+4*tfd -> nb_strips);
	uint16_t *buffer = malloc(taille_buffer*sizeof(uint16_t));
	
	// Endianness + Indentification
	buffer[0] = 0x4949;
	buffer[1] = 0x002A;
	
	// Ptr IFD
	buffer[2] = 0x0008;
	buffer[3] = ZERO;
	
	// Nombre d'entrées
	buffer[4] = 0x000C;
	
	// Image Width
	buffer[5] = 0x0100;
	if (tfd -> width <= 0xFFFF){
		buffer[6] = SHORT;
		buffer[7] = UN;
		buffer[8] = ZERO;
		buffer[9] = tfd -> width;
		buffer[10] = ZERO;
	}else{
		buffer[6] = LONG;
		buffer[7] = UN;
		buffer[8] = ZERO;
		buffer[9] =  tfd -> width;
		buffer[10] = (tfd -> width >> 16);
	}

	// Image Length
	buffer[11] = 0x0101;
	if (tfd -> width <= 0xFFFF){
		buffer[12] = SHORT;
		buffer[13] = UN;
		buffer[14] = ZERO;
		buffer[15] = tfd -> height;
		buffer[16] = ZERO;
	}else{
		buffer[12] = LONG;
		buffer[13] = UN;
		buffer[14] = ZERO;
		buffer[15] = tfd -> height;
		buffer[16] = (tfd -> height  >> 16);
	}

	// BitsPerSample
	buffer[17] = 0x0102;
	buffer[18] = SHORT;
	buffer[19] = TROIS;
	buffer[20] = ZERO;
	buffer[21] = 0x009E;
	buffer[22] = ZERO;
	
	// Compression
	buffer[23] = 0x0103;
	buffer[24] = SHORT;
	buffer[25] = UN;
	buffer[26] = ZERO;
	buffer[27] = UN;
	buffer[28] = ZERO;

	// PhotometricInterpretation
	buffer[29] = 0x0106;
	buffer[30] = SHORT;
	buffer[31] = UN;
	buffer[32] = ZERO;
	buffer[33] = DEUX;
	buffer[34] = ZERO;

	// StripOffsets
	buffer[35] = 0x0111;
	buffer[36] = LONG;
	buffer[37] = tfd -> nb_strips;
	buffer[38] = tfd -> nb_strips >> 16;
	buffer[39] = 0x00B4;
	buffer[40] = ZERO;

	// SamplesPerPixel
	buffer[41] = 0x0115;
	buffer[42] = SHORT;
	buffer[43] = UN;
	buffer[44] = ZERO;
	buffer[45] = TROIS;
	buffer[46] = ZERO;

	// RowsPerStrip
	buffer[47] = 0x0116;
	buffer[48] = LONG;
	buffer[49] = UN;
	buffer[50] = ZERO;
	buffer[51] = tfd -> rows_per_strip;
	buffer[52] = tfd -> rows_per_strip >> 16;

	// StripByteCounts
	buffer[53] = 0x0117;
	buffer[54] = LONG;
	buffer[55] = tfd -> nb_strips;
	buffer[56] = tfd -> nb_strips >> 16;
	buffer[57] = (0x00B4 + tfd -> nb_strips * 0x4);
	buffer[58] = ZERO;

	// XResolution
	buffer[59] = 0x011a;
	buffer[60] = RATI;
	buffer[61] = UN;
	buffer[62] = ZERO;
	buffer[63] = 0x00A4;
	buffer[64] = ZERO;

	// YResolution
	buffer[65] = 0x011b;
	buffer[66] = RATI;
	buffer[67] = UN;
	buffer[68] = ZERO;
	buffer[69] = 0x00AC;
	buffer[70] = ZERO;

	// ResolutionUnit
	buffer[71] = 0x0128;
	buffer[72] = SHORT;
	buffer[73] = UN;
	buffer[74] = ZERO;
	buffer[75] = DEUX;
	buffer[76] = ZERO;

	// Offset suivant
	buffer[77] = ZERO;
	buffer[78] = ZERO;

	// --> Ptr BitsPerSample
	buffer[79] = 0x0008;
	buffer[80] = 0x0008;
	buffer[81] = 0x0008;

	// --> Ptr XResolution & YResolution
	buffer[82] = 0x0064;
	buffer[83] = ZERO;
	buffer[84] = UN;
	buffer[85] = ZERO;

	buffer[86] = 0x0064;
	buffer[87] = ZERO;
	buffer[88] = UN;
	buffer[89] = ZERO;

	// --> Ptr StripOffsets
	uint32_t ptr_ligne = 0x00B4+8* tfd -> nb_strips;
	uint32_t taille_ligne = tfd -> rows_per_strip*tfd->width*3;
	
	for (uint32_t i =0; i < tfd -> nb_strips; i ++){
		buffer[90+2*i] = ptr_ligne;
		buffer[90+2*i+1] = (ptr_ligne >> 16);
		ptr_ligne += taille_ligne;
	}

	uint32_t indice_dans_buffer = 90+2*tfd -> nb_strips;
	// --> Ptr StripByteCounts
	for (uint32_t i =0; i < tfd -> nb_strips-1; i++){
		buffer[indice_dans_buffer+2*i] = taille_ligne;
		buffer[indice_dans_buffer+2*i+1] = (taille_ligne >> 16);
	}
	
	// Taille de la dernière ligne
	indice_dans_buffer = indice_dans_buffer+2*(tfd -> nb_strips-1);
	taille_ligne = (tfd -> height % tfd -> rows_per_strip)*tfd->width*3;
	buffer[indice_dans_buffer] = taille_ligne;
	buffer[indice_dans_buffer+1] = (taille_ligne >> 16);

	fwrite(buffer,sizeof(uint16_t),taille_buffer,tfd -> file);
	
	return tfd;
	
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

