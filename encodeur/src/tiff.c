
#include "tiff.h"
#include "common.h"



// Macros pour l'écriture dans le fichier

#define SHORT 0x0003
#define LONG 0x0004
#define RATI 0x0005

#define ZERO 0x0000
#define UN 0x0001
#define DEUX 0x0002
#define TROIS 0x0003


#define CHECK_READ_SIZE(s)  if (read_size != s){perror("Erreur ");exit(1);}
#define TREAT_ENDIANESS_32(valeur,is_le)  valeur = (is_le?valeur:((valeur>>24)&0xff) | ((valeur<<8)&0xff0000) | ((valeur>>8)&0xff00) | ((valeur<<24)&0xff000000))
#define TREAT_ENDIANESS_16(valeur,is_le)  valeur = (is_le?valeur:((valeur>>24)&0xff) | ((valeur<<8)&0xff0000) | ((valeur>>8)&0xff00) | ((valeur<<24)&0xff000000))

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

/* lit une ifd et met à jour les bons champs dans tfd */
void read_ifd(struct tiff_file_desc *tfd){
	uint16_t tag;
	uint16_t type;
	uint32_t count;
	uint32_t value;
	size_t read_size;
	
	// Tag d'identification
	read_size = fread(&tag,1,2, tfd->file);
	CHECK_READ_SIZE(2);
	TREAT_ENDIANESS_16(tag,tfd ->is_le);

	// Type
	read_size = fread(&type,1,2, tfd->file);
	CHECK_READ_SIZE(2);
	TREAT_ENDIANESS_16(type,tfd ->is_le);

	// Nombre des valeurs
	read_size = fread(&count,1,4, tfd->file);
	CHECK_READ_SIZE(4);
	TREAT_ENDIANESS_32(count,tfd ->is_le);

	// Valeur
	read_size = fread(&value,1,4, tfd->file);
	CHECK_READ_SIZE(4);
	TREAT_ENDIANESS_32(value,tfd ->is_le);

	
	uint32_t current_pos;
	
	switch(tag){
		// ImageWidth
	case 0x0100:
		tfd -> width = value;
		break;

		// ImageLength
	case 0x0101:
		tfd -> height = value;
		break;

		// BitsPerSample
	case 0x0102:
		tfd -> bits_per_sample = value;
		break;

		// Compression
	case 0x0103:
		tfd -> compression = value;
		break;

		// PhotometricInterpretation
	case 0x0106:
		tfd -> photometric_interpretation = value;
		break;

		// StripOffsets
	case 0x0111:
		tfd -> nb_strips = count;
		tfd -> strip_offsets_count = count;
		tfd -> strip_offsets = malloc(count*sizeof(uint32_t));
		if (((count > 1) && (type == LONG)) || ((count > 2) && (type == SHORT))){
			

			current_pos = ftell(tfd ->file);
			fseek(tfd ->file,value,0);
			
			for (uint32_t i =0; i <count; ++i){
				read_size = fread(&(tfd -> strip_offsets[i]),1,4, tfd ->file);
				CHECK_READ_SIZE(4);
				TREAT_ENDIANESS_32(tfd -> strip_offsets[i],tfd ->is_le);
			}
			fseek(tfd ->file,current_pos,0);
			
		}else if ((type == SHORT) && (count == 2)){
			tfd -> strip_offsets[0] = value >> 16;
			tfd -> strip_offsets[1] = value & 0xFF;
					
		}else{
			tfd -> strip_offsets[0] = value;
		}
		tfd -> current_line = tfd -> strip_offsets[0];
		break;

		// SamplesPerPixel
	case 0x0115:
		tfd -> samples_per_pixels = value;
		break;

		// RowsPerStrip
	case 0x0116:
		tfd -> rows_per_strip = value;
		break;

		// StripByteCounts
	case 0x0117:

		tfd -> strip_bytes_count = count;
		tfd -> strip_bytes = malloc(count*sizeof(uint32_t));
		if (((count > 1) && (type == LONG)) || ((count > 2) && (type == SHORT))){
			

			current_pos = ftell(tfd ->file);
			fseek(tfd ->file,value,0);

			for (uint32_t i =0; i <count; ++i){
				read_size = fread(&(tfd -> strip_bytes[i]),1,4, tfd ->file);
				CHECK_READ_SIZE(4);
				TREAT_ENDIANESS_32(tfd -> strip_bytes[i],tfd ->is_le);
			}
			fseek(tfd ->file,current_pos,0);
		
		}else if ((type == SHORT) && (count == 2)){
			tfd -> strip_bytes[0] = value >> 16;
			tfd -> strip_bytes[1] = value & 0xFF;
					
		}else{
			tfd -> strip_bytes[0] = value;
		}

		break;

		// XResolution
	case 0x011a:
		current_pos = ftell(tfd ->file);

		fseek(tfd ->file,value,0);
		read_size = fread(&(tfd ->x_resolution_num),1,4, tfd ->file);
		CHECK_READ_SIZE(4);
		TREAT_ENDIANESS_32(tfd ->x_resolution_num,tfd ->is_le);
		read_size = fread(&(tfd ->x_resolution_denum),1,4, tfd ->file);
		CHECK_READ_SIZE(4);
		TREAT_ENDIANESS_32(tfd ->x_resolution_denum,tfd ->is_le);
		
		fseek(tfd ->file,current_pos,0);
		
		break;

		// YResolution
	case 0x011b:
		current_pos = ftell(tfd ->file);

		fseek(tfd ->file,value,0);
		read_size = fread(&(tfd ->y_resolution_num),1,4, tfd ->file);
		CHECK_READ_SIZE(4);
		TREAT_ENDIANESS_32(tfd ->y_resolution_num,tfd ->is_le);
		read_size = fread(&(tfd ->y_resolution_denum),1,4, tfd ->file);
		CHECK_READ_SIZE(4);
		TREAT_ENDIANESS_32(tfd ->y_resolution_denum,tfd ->is_le);
		
		fseek(tfd ->file,current_pos,0);
		break;

		// ResolutionUnit
	case 0x0128:
		tfd -> resolution_unit = value;
		break;
	default:
		NULL;
	}
}

/* Renvoie un pointeur vers le tiff_file_desc correspondant au fichier tiff de * path file_name après la lecture du header
 */
struct tiff_file_desc *init_tiff_file_read (const char *file_name, uint32_t *width, uint32_t *height, uint32_t *row_per_strip)
{
	struct tiff_file_desc *tfd = malloc(sizeof(struct tiff_file_desc));
	

	tfd -> file = fopen(file_name,"rb");
	if (tfd -> file == NULL){
		perror("Erreur: ");
		exit(1);
	}

	uint16_t *buffer = malloc(sizeof(uint16_t));
	size_t read_size;
	
	// Endianess
	read_size = fread(buffer,1,2, tfd->file);
	CHECK_READ_SIZE(2);
	if (*buffer == 0x4949){
		tfd -> is_le = true;
	} else if (*buffer == 0x4D4D){
		tfd -> is_le = false;
	} else {
		printf("ERROR : Invalid TIFF file\n");
                exit(1);
	}

	// 42
	read_size = fread(buffer,1,2, tfd->file);
	CHECK_READ_SIZE(2);
	if (*buffer != 0x2a){
		printf("ERROR : Not a TIFF file\n");
                exit(1);
	}
	
	// Ptr IFD
	read_size = fread(&(tfd ->ptr_ifd),1,4, tfd->file);
	CHECK_READ_SIZE(4);
	TREAT_ENDIANESS_16(tfd ->ptr_ifd,tfd ->is_le);

	// IFD count
	fseek(tfd -> file, tfd -> ptr_ifd, 0);
	read_size = fread(&(tfd ->ifd_count),1,2, tfd->file);
	CHECK_READ_SIZE(2);
	TREAT_ENDIANESS_16(tfd ->ifd_count,tfd ->is_le);
	
	for(uint32_t i=0; i <tfd -> ifd_count;i++){
		read_ifd(tfd);
	}

        *width = tfd -> width;
        *height = tfd -> height;
        *row_per_strip = tfd -> rows_per_strip;

	tfd -> current_line = 0;
	tfd -> next_pos_mcu = 0;
	tfd -> row_size = tfd -> width * 3;
			  
	SAFE_FREE(buffer);
	return tfd;
}

/* Ferme le fichier associé à la structure tiff_file_desc passée en
 * paramètre et désalloue la mémoire occupée par cette structure. */
void close_tiff_file(struct tiff_file_desc *tfd)
{
        if (tfd != NULL && tfd->file != NULL)
                fclose(tfd->file);

	SAFE_FREE(tfd);
}


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

	uint32_t ptr_ligne = 0x00B4+8* tfd -> nb_strips;
	tfd -> strip_offsets_count = tfd -> nb_strips;
	tfd -> strip_offsets = malloc(tfd -> strip_offsets_count*sizeof(uint32_t));

	buffer[35] = 0x0111;
	buffer[36] = LONG;
	buffer[37] = tfd -> nb_strips;
	buffer[38] = tfd -> nb_strips >> 16;


	if (tfd -> strip_offsets_count > 1){
		buffer[39] = 0x00B4;
		buffer[40] = ZERO;
	} else {
		buffer[39] = ptr_ligne;
		buffer[40] = (ptr_ligne >> 16);
		tfd -> strip_offsets[0] = ptr_ligne;
	}


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

	uint32_t hauteur_ligne = tfd -> height % tfd -> rows_per_strip;

        if (tfd -> height > 0  && hauteur_ligne == 0)
                hauteur_ligne = tfd -> rows_per_strip;

	uint32_t taille_ligne;
	
	tfd -> strip_bytes_count = tfd -> nb_strips;
	tfd -> strip_bytes = malloc(tfd -> strip_bytes_count*sizeof(uint32_t));
	

	
	if (tfd -> strip_bytes_count > 1){
		taille_ligne = tfd -> rows_per_strip*tfd->width*3;
		buffer[57] = (0x00B4 + tfd -> nb_strips * 0x4);
		buffer[58] = ZERO;
	} else {
		taille_ligne = hauteur_ligne * tfd->width*3;
		buffer[57] = taille_ligne;
		buffer[58] = (taille_ligne >> 16);
	}
	


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

	
	// Initialisation des informations necéssaires pour l'écriture au format TIFF
	tfd -> current_line = 0;

	tfd -> size_line = taille_ligne;
	tfd -> row_size = tfd->width*3;
	// Initialisation de la position de la 1ere mcu
	tfd -> next_pos_mcu = 0;
	
	for (uint32_t i =0; i < tfd -> nb_strips; i ++){
		buffer[90+2*i] = ptr_ligne;
		buffer[90+2*i+1] = (ptr_ligne >> 16);

		tfd -> strip_offsets[i] = ptr_ligne;

		ptr_ligne += taille_ligne;
	}

	uint32_t indice_dans_buffer = 90+2*tfd -> nb_strips;
	// --> Ptr StripByteCounts

	for (uint32_t i =0; i < tfd -> nb_strips - 1; i++){
		buffer[indice_dans_buffer+2*i] = taille_ligne;
		buffer[indice_dans_buffer+2*i+1] = (taille_ligne >> 16);
		tfd -> strip_bytes[i] = taille_ligne;
	}
	
	// Taille de la dernière ligne
	indice_dans_buffer = indice_dans_buffer+2*(tfd -> nb_strips-1);

       

	taille_ligne = hauteur_ligne * tfd->width*3;
	
	
	buffer[indice_dans_buffer] = taille_ligne;
	buffer[indice_dans_buffer+1] = (taille_ligne >> 16);
	tfd -> strip_bytes[tfd -> nb_strips - 1] = taille_ligne;
	

	fwrite(buffer,sizeof(uint16_t),taille_buffer,tfd -> file);

	SAFE_FREE(buffer);
	
	return tfd;
	
}


/* Lit une MCU composée de nb_blocks_h et nb_blocks_v 
 * blocs 8x8  en horizontal et en vertical à partir du 
 * fichier fichier TIFF représenté par la structure 
 * tiff_file_desc tfd. */
void read_tiff_file (struct tiff_file_desc *tfd, uint32_t *mcu_rgb,
			  uint8_t nb_blocks_h,
			  uint8_t nb_blocks_v){

	uint32_t read_size;
	uint32_t current_position;
	

	printf("Nb  Strips   : %u \n",tfd -> nb_strips); 
	printf("CurrntStrp   : %u \n",tfd -> current_line);
	if ((tfd -> current_line < (tfd -> nb_strips-1)) && (tfd -> strip_offsets[tfd -> current_line] + tfd -> next_pos_mcu + (nb_blocks_v*BLOCK_DIM-1)*tfd -> row_size >= tfd -> strip_offsets[tfd -> current_line+1])){
		
		tfd -> current_line++;
		tfd -> next_pos_mcu = 0;
		printf("New Line   : %u \n",tfd -> current_line); 
		
	}

	current_position= tfd -> strip_offsets[tfd -> current_line] + tfd -> next_pos_mcu;

	
	uint32_t nb_read_h, nb_read_v;
	if (((tfd -> next_pos_mcu%tfd -> row_size) + nb_blocks_h*BLOCK_DIM*3 > tfd -> row_size)){
		nb_read_h = (nb_blocks_h*BLOCK_DIM*3-((tfd -> next_pos_mcu%tfd -> row_size) + nb_blocks_h*BLOCK_DIM*3-tfd -> row_size))/3;
		tfd -> next_pos_mcu += 3*nb_read_h+(nb_blocks_v*BLOCK_DIM-1)*tfd -> row_size;
	} else { 
		nb_read_h = nb_blocks_h*BLOCK_DIM;
		tfd -> next_pos_mcu += 3*nb_read_h;
	}

	nb_read_v = ((nb_blocks_v*BLOCK_DIM) > (tfd -> strip_bytes[tfd -> current_line] / tfd -> row_size)?(tfd -> strip_bytes[tfd -> current_line] / tfd -> row_size) : (nb_blocks_v*BLOCK_DIM));
	
	printf("Strip Bytes : %u \n",tfd -> strip_bytes[tfd -> current_line]); 
	printf("nb_read_v   : %u \n",nb_read_v);
	printf("nb_read_h   : %u \n",nb_read_h);
	
	for(uint32_t i=0; i < nb_read_v;i++){
		fseek(tfd -> file, current_position, 0);
		
		for (uint32_t j=0; j < nb_read_h;j++){
			
			read_size = fread(&(mcu_rgb[i*nb_blocks_h*BLOCK_DIM+j]),1,3, tfd->file);
			CHECK_READ_SIZE(3);
			TREAT_ENDIANESS_32(mcu_rgb[i*nb_blocks_h*BLOCK_DIM+j],!tfd ->is_le);// Attention au ! avant tfd ->is_le
			mcu_rgb[i*nb_blocks_h*BLOCK_DIM+j] = mcu_rgb[i*nb_blocks_h*BLOCK_DIM+j] >> 8;
		}
		//On remplie le reste de la ligne de la MCU avec des zéros
		for (uint32_t j=nb_read_h; j < nb_blocks_h*BLOCK_DIM;j++){
			mcu_rgb[i*nb_blocks_h*BLOCK_DIM+j] = 0;
		}
		
		current_position += tfd -> row_size;	
	}
	
	//On remplie le reste des lignes de la  MCU avec des zéros
	for(uint32_t i=nb_read_v; i < nb_blocks_v*BLOCK_DIM;i++){
		for (uint32_t j=0; j < nb_blocks_h*BLOCK_DIM;j++){
			mcu_rgb[i*nb_blocks_h*BLOCK_DIM+j] = 0;
		}
	}
	
	

}

/* Ecrit le contenu de la MCU passée en paramètre dans le fichier TIFF
 * représenté par la structure tiff_file_desc tfd. nb_blocks_h et
 * nb_blocks_v représentent les nombres de blocs 8x8 composant la MCU
 * en horizontal et en vertical. */
void write_tiff_file (struct tiff_file_desc *tfd,
			 uint32_t *mcu_rgb,
			 uint8_t nb_blocks_h,
			 uint8_t nb_blocks_v)
{


	//On passe à la strip suivante s'il n'y a plus de place pour mettre la MCU
	if ((tfd -> current_line < (tfd -> nb_strips-1)) &&(tfd -> strip_offsets[tfd -> current_line] + tfd -> next_pos_mcu + (nb_blocks_v*BLOCK_DIM-1)*tfd -> row_size >= tfd -> strip_offsets[tfd -> current_line+1])){
		tfd -> current_line++;
		tfd -> next_pos_mcu = 0;
	}
	uint32_t current_position = tfd -> strip_offsets[tfd -> current_line] + tfd -> next_pos_mcu;

	// calcul du nombre de pixel RGB à écrire dans le fichier en fonction de row_size
	uint32_t nb_write_h, nb_write_v;
	if (((tfd -> next_pos_mcu%tfd -> row_size) + nb_blocks_h*BLOCK_DIM*3 > tfd -> row_size)){
		nb_write_h = (nb_blocks_h*BLOCK_DIM*3-((tfd -> next_pos_mcu%tfd -> row_size) + nb_blocks_h*BLOCK_DIM*3-tfd -> row_size))/3;
		tfd -> next_pos_mcu += 3*nb_write_h+(nb_blocks_v*BLOCK_DIM-1)*tfd -> row_size;
	} else { 
		nb_write_h = nb_blocks_h*BLOCK_DIM;
		tfd -> next_pos_mcu += 3*nb_write_h;
	}

	nb_write_v = ((nb_blocks_v*BLOCK_DIM) > (tfd -> strip_bytes[tfd -> current_line] / tfd -> row_size)?(tfd -> strip_bytes[tfd -> current_line] / tfd -> row_size) : (nb_blocks_v*BLOCK_DIM));
	

	
	// tableau intermédiare pour récupérer les valeurs RGB
	uint8_t *rgb_mcu_row = malloc(sizeof(uint8_t)*nb_blocks_h*BLOCK_DIM*3);
	
	for(uint32_t i = 0; i < nb_write_v; i++) {
		
		int k=0;
		for(uint32_t j = 0; j < nb_write_h; j++) {
			rgb_mcu_row[k] = mcu_rgb[i*nb_blocks_h*BLOCK_DIM+j] >> 16;
			rgb_mcu_row[k+1] = mcu_rgb[i*nb_blocks_h*BLOCK_DIM+j] >> 8;
			rgb_mcu_row[k+2] = mcu_rgb[i*nb_blocks_h*BLOCK_DIM+j];
			k +=3;
		}
		fseek(tfd -> file, current_position, 0);
		fwrite(rgb_mcu_row, sizeof(uint8_t), 3*nb_write_h, tfd -> file);
		current_position += tfd -> row_size;
	}
	
	//tfd -> next_pos_mcu += nb_rgb_to_write;
	
	SAFE_FREE(rgb_mcu_row);
}

