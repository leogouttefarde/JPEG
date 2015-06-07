
#include "tiff.h"
#include "common.h"


/* TIFF defines */

#define BYTE    0x0001
#define ASCII   0x0002
#define SHORT   0x0003
#define LONG    0x0004
#define RATI    0x0005

#define ZERO    0x0000
#define UN      0x0001
#define DEUX    0x0002
#define TROIS   0x0003


#define _BYTE(c)            (c & 0xFF)
#define bytes2long_be(b)    (_BYTE(b[0]) << 24 | _BYTE(b[1]) << 16 | _BYTE(b[2]) << 8 | _BYTE(b[3]))
#define bytes2long_le(b)    (_BYTE(b[3]) << 24 | _BYTE(b[2]) << 16 | _BYTE(b[1]) << 8 | _BYTE(b[0]))
#define bytes2long(b, le)   (le ? bytes2long_le(b) : bytes2long_be(b))

#define bytes2short_be(b)   (_BYTE(b[0]) << 8 | _BYTE(b[1]))
#define bytes2short_le(b)   (_BYTE(b[1]) << 8 | _BYTE(b[0]))
#define bytes2short(b, le)  (le ? bytes2short_le(b) : bytes2short_be(b))


/*
 * Internal TIFF structure informations
 */
struct tiff_file_desc {

        /* This tiff file's pointer */
        FILE *file;

        /* Indicates if this tiff uses little endian or not */
        bool is_le;

        /* IFD informations */
        uint32_t ifd_offset;
        uint32_t ifd_count;

        /* Dimensions */
        uint32_t width;
        uint32_t height;

        /* tiff informations */
        uint16_t bits_per_sample;
        uint16_t compression;
        uint16_t photometric_interpretation;
        uint16_t samples_per_pixels;
        uint32_t rows_per_strip;

        /* Strip informations */
        uint32_t nb_strips;
        uint32_t *strip_offsets;

        /* Strip size informations */
        uint32_t strip_bytes_count;
        uint32_t *strip_bytes;

        /* Internal informations */
        uint32_t current_line;
        uint32_t next_pos_mcu;
        uint32_t size_line;
        uint32_t row_size;
        uint32_t read_lines;
};


static uint16_t read_short(struct tiff_file_desc *tfd, bool *error);

static uint32_t read_long(struct tiff_file_desc *tfd, bool *error);

static void read_ifd_entry(struct tiff_file_desc *tfd, bool *error);


/*
 * Initialisation de la lecture d'un fichier TIFF, avec les sorties suivantes :
 *  - width : la largeur de l'image lue
 *  - height: la hauteur de l'image lue
 */
struct tiff_file_desc *init_tiff_read (const char *path, uint32_t *width, uint32_t *height)
{
        FILE *file = NULL;
        bool error = false;
        struct tiff_file_desc *tfd = calloc(1, sizeof(struct tiff_file_desc));

        if (tfd == NULL)
                return NULL;


        file = fopen(path, "rb");
        if (file == NULL)
                return NULL;

        tfd->file = file;


        uint16_t endian;

        /* Endianness detection */
        endian = read_short(tfd, &error);

        if (endian == 0x4949)
                tfd->is_le = true;

        else if (endian == 0x4D4D)
                tfd->is_le = false;

        else
                error = true;


        /* 42 tiff marker */
        read_short(tfd, &error);


        /* IFD offset */
        tfd->ifd_offset = read_long(tfd, &error);

        /* Go to our IFD offset */
        fseek(file, tfd->ifd_offset, SEEK_SET);

        /* IFD count */
        tfd->ifd_count = read_short(tfd, &error);


        /* Default compression to none */
        tfd->compression = 1;

        for(uint32_t i = 0; i < tfd->ifd_count; i++)
                read_ifd_entry(tfd, &error);


        tfd->current_line = 0;
        tfd->read_lines = 0;


        uint16_t samples = tfd->samples_per_pixels;
        uint16_t photo = tfd->photometric_interpretation;

        /* Check tiff validity / compatibility */
        if (tfd->strip_offsets == NULL
            || tfd->width == 0
            || tfd->height == 0
            || (samples != 1 && samples != 3 && samples != 4)
            || tfd->compression != 1
            || tfd->bits_per_sample != 8
            || (photo != 1 && photo != 2))
                error = true;


        /* Cleanup on error */
        if (error) {
                close_tiff_file(tfd);
                tfd = NULL;
        }

        else {
                *width = tfd->width;
                *height = tfd->height;

                /* Move the file to the first strip to read */
                fseek(tfd->file, tfd->strip_offsets[tfd->current_line], SEEK_SET);
        }


        return tfd;
}

bool read_tiff_line(struct tiff_file_desc *tfd, uint32_t *line_rgb)
{
        bool error = false;

        if (tfd->rows_per_strip > 0)
        if (tfd->read_lines >= tfd->rows_per_strip) {

                if (++tfd->current_line < tfd->nb_strips) {
                        fseek(tfd->file, tfd->strip_offsets[tfd->current_line], SEEK_SET);

                        tfd->read_lines = 0;
                }
        }


        uint32_t i = 0;
        char buf[4];
        memset(buf, 0, sizeof(buf));
        size_t ret;

        for (uint32_t w = 0; w < tfd->width; w++) {

                ret = fread(buf, 1, tfd->samples_per_pixels, tfd->file);

                if (ret != tfd->samples_per_pixels) {
                        error = true;
                        break;
                }

                if (tfd->samples_per_pixels == 1)
                        buf[1] = buf[2] = buf[0];

                line_rgb[i++] = ((buf[0] & 0xFF) << 16) | ((buf[1] & 0xFF) << 8) | (buf[2] & 0xFF);
        }

        tfd->read_lines++;


        return error;
}

static uint16_t read_short(struct tiff_file_desc *tfd, bool *error)
{
        uint16_t value = -1;

        if (tfd != NULL && tfd->file != NULL && !*error) {
                uint8_t buf[2];
                size_t ret;

                ret = fread(buf, 1, sizeof(buf), tfd->file);

                if (ret != sizeof(buf))
                        *error = true;

                else
                        value = bytes2short(buf, tfd->is_le);
        }
        else
                *error = true;

        return value;
}

static uint32_t read_long(struct tiff_file_desc *tfd, bool *error)
{
        uint32_t value = -1;

        if (tfd != NULL && tfd->file != NULL && !*error) {
                uint8_t buf[4];
                size_t ret;

                ret = fread(buf, 1, sizeof(buf), tfd->file);

                if (ret != sizeof(buf))
                        *error = true;

                else
                        value = bytes2long(buf, tfd->is_le);
        }
        else
                *error = true;

        return value;
}

/* lit une entrée ifd et met à jour les bons champs dans tfd */
static void read_ifd_entry(struct tiff_file_desc *tfd, bool *error)
{
        if (error == NULL || *error)
                return;


        uint16_t tag;
        uint16_t type;
        uint32_t count;
        uint32_t value, next_value;
        uint32_t cur_pos;
        uint8_t buf[4];
        size_t ret;

        /* Identification tag */
        tag = read_short(tfd, error);

        /* Type */
        type = read_short(tfd, error);

        /* Number of values */
        count = read_long(tfd, error);


        /* Value */
        ret = fread(buf, 1, sizeof(buf), tfd->file);

        if (ret != sizeof(buf))
                *error = true;


        switch (type) {
        case BYTE:
                value = buf[0];
                break;

        case SHORT:
                value = bytes2short(buf, tfd->is_le);

                uint8_t *next_buf = &buf[2];
                next_value = bytes2short(next_buf, tfd->is_le);
                break;

        case LONG:
                value = bytes2long(buf, tfd->is_le);
                break;

        /* Unused values */
        case ASCII:
        case RATI:
        default:
                value = 0;
        }


        switch(tag) {

        /* ImageWidth */
        case 0x0100:
                tfd->width = value;
                break;

        /* ImageLength */
        case 0x0101:
                tfd->height = value;
                break;

        /* BitsPerSample */
        case 0x0102:

                if (count <= 2)
                        tfd->bits_per_sample = value;
                else {
                        cur_pos = ftell(tfd->file);
                        fseek(tfd->file, value, SEEK_SET);

                        tfd->bits_per_sample = read_short(tfd, error);

                        fseek(tfd->file, cur_pos, SEEK_SET);
                }
                break;

        /* Compression */
        case 0x0103:
                tfd->compression = value;
                break;

        /* PhotometricInterpretation */
        case 0x0106:
                tfd->photometric_interpretation = value;
                break;

        /* StripOffsets */
        case 0x0111:
                tfd->nb_strips = count;
                tfd->strip_offsets = calloc(count, sizeof(uint32_t));

                if (tfd->strip_offsets == NULL) {
                        *error = true;
                        return;
                }


                if ((count > 1 && type == LONG) || (count > 2 && type == SHORT)) {

                        cur_pos = ftell(tfd->file);
                        fseek(tfd->file, value, SEEK_SET);

                        for (uint32_t i = 0; i < count; ++i)
                                tfd->strip_offsets[i] = read_long(tfd, error);

                        fseek(tfd->file, cur_pos, SEEK_SET);
                        
                } else if (type == SHORT && count == 2) {
                        tfd->strip_offsets[0] = value;
                        tfd->strip_offsets[1] = next_value;
                                        
                } else
                        tfd->strip_offsets[0] = value;

                break;


        /* SamplesPerPixel */
        case 0x0115:
                tfd->samples_per_pixels = value;
                break;

        /* RowsPerStrip */
        case 0x0116:
                /*
                 * Big values mean infinity,
                 * in which case there are no strips
                 */
                if (value > 0xFFFFFF)
                        value = 0;

                tfd->rows_per_strip = value;
                break;

        /* StripByteCounts */
        case 0x0117:

                tfd->strip_bytes_count = count;
                tfd->strip_bytes = calloc(count, sizeof(uint32_t));

                if (tfd->strip_bytes == NULL) {
                        *error = true;
                        return;
                }


                if ((count > 1 && type == LONG) || (count > 2 && type == SHORT)) {

                        cur_pos = ftell(tfd->file);
                        fseek(tfd->file, value, SEEK_SET);

                        for (uint32_t i = 0; i < count; ++i)
                                tfd->strip_bytes[i] = read_long(tfd, error);

                        fseek(tfd->file, cur_pos, SEEK_SET);
                
                } else if (type == SHORT && count == 2) {
                        tfd->strip_bytes[0] = value;
                        tfd->strip_bytes[1] = next_value;
                }

                else
                        tfd->strip_bytes[0] = value;
        }
}

/* Ferme le fichier associé à la structure tiff_file_desc passée en
 * paramètre et désalloue la mémoire occupée par cette structure. */
void close_tiff_file(struct tiff_file_desc *tfd)
{
        if (tfd != NULL) {
                if (tfd->file != NULL)
                        fclose(tfd->file);

                SAFE_FREE(tfd->strip_offsets);
                SAFE_FREE(tfd->strip_bytes);
        }

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
        
        tfd->file = fopen(file_name,"wb");
        if (tfd->file == NULL){
                perror("Erreur: ");
                exit(1);
        }
        
        tfd->width = width;
        tfd->height = height;
        tfd->rows_per_strip = row_per_strip;
        
        tfd->nb_strips = tfd->height / tfd->rows_per_strip;
        tfd->nb_strips = ((tfd->nb_strips * tfd->rows_per_strip) < tfd->height ? tfd->nb_strips + 1 : tfd->nb_strips);

        // écriture du header dans le fichier
        //===================================
        uint32_t taille_buffer = (90+4*tfd->nb_strips);
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
        if (tfd->width <= 0xFFFF){
                buffer[6] = SHORT;
                buffer[7] = UN;
                buffer[8] = ZERO;
                buffer[9] = tfd->width;
                buffer[10] = ZERO;
        }else{
                buffer[6] = LONG;
                buffer[7] = UN;
                buffer[8] = ZERO;
                buffer[9] =  tfd->width;
                buffer[10] = (tfd->width >> 16);
        }

        // Image Length
        buffer[11] = 0x0101;
        if (tfd->width <= 0xFFFF){
                buffer[12] = SHORT;
                buffer[13] = UN;
                buffer[14] = ZERO;
                buffer[15] = tfd->height;
                buffer[16] = ZERO;
        }else{
                buffer[12] = LONG;
                buffer[13] = UN;
                buffer[14] = ZERO;
                buffer[15] = tfd->height;
                buffer[16] = (tfd->height  >> 16);
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

        uint32_t ptr_ligne = 0x00B4+8* tfd->nb_strips;
        tfd->strip_offsets = malloc(tfd->nb_strips*sizeof(uint32_t));

        buffer[35] = 0x0111;
        buffer[36] = LONG;
        buffer[37] = tfd->nb_strips;
        buffer[38] = tfd->nb_strips >> 16;


        if (tfd->nb_strips > 1){
                buffer[39] = 0x00B4;
                buffer[40] = ZERO;
        } else {
                buffer[39] = ptr_ligne;
                buffer[40] = (ptr_ligne >> 16);
                tfd->strip_offsets[0] = ptr_ligne;
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
        buffer[51] = tfd->rows_per_strip;
        buffer[52] = tfd->rows_per_strip >> 16;

        // StripByteCounts

        uint32_t hauteur_ligne = tfd->height % tfd->rows_per_strip;

        if (tfd->height > 0  && hauteur_ligne == 0)
                hauteur_ligne = tfd->rows_per_strip;

        uint32_t taille_ligne;
        
        tfd->strip_bytes_count = tfd->nb_strips;
        tfd->strip_bytes = malloc(tfd->strip_bytes_count*sizeof(uint32_t));
        

        
        if (tfd->strip_bytes_count > 1){
                taille_ligne = tfd->rows_per_strip*tfd->width*3;
                buffer[57] = (0x00B4 + tfd->nb_strips * 0x4);
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
        tfd->current_line = 0;

        tfd->size_line = taille_ligne;
        tfd->row_size = tfd->width*3;
        // Initialisation de la position de la 1ere mcu
        tfd->next_pos_mcu = 0;
        
        for (uint32_t i =0; i < tfd->nb_strips; i ++){
                buffer[90+2*i] = ptr_ligne;
                buffer[90+2*i+1] = (ptr_ligne >> 16);

                tfd->strip_offsets[i] = ptr_ligne;

                ptr_ligne += taille_ligne;
        }

        uint32_t indice_dans_buffer = 90+2*tfd->nb_strips;
        // --> Ptr StripByteCounts

        for (uint32_t i =0; i < tfd->nb_strips - 1; i++){
                buffer[indice_dans_buffer+2*i] = taille_ligne;
                buffer[indice_dans_buffer+2*i+1] = (taille_ligne >> 16);
                tfd->strip_bytes[i] = taille_ligne;
        }
        
        // Taille de la dernière ligne
        indice_dans_buffer = indice_dans_buffer+2*(tfd->nb_strips-1);

       

        taille_ligne = hauteur_ligne * tfd->width*3;
        
        
        buffer[indice_dans_buffer] = taille_ligne;
        buffer[indice_dans_buffer+1] = (taille_ligne >> 16);
        tfd->strip_bytes[tfd->nb_strips - 1] = taille_ligne;
        

        fwrite(buffer,sizeof(uint16_t),taille_buffer,tfd->file);

        SAFE_FREE(buffer);


        return tfd;
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
        if ((tfd->current_line < (tfd->nb_strips-1)) &&(tfd->strip_offsets[tfd->current_line] + tfd->next_pos_mcu + (nb_blocks_v*BLOCK_DIM-1)*tfd->row_size >= tfd->strip_offsets[tfd->current_line+1])){
                tfd->current_line++;
                tfd->next_pos_mcu = 0;
        }
        uint32_t current_position = tfd->strip_offsets[tfd->current_line] + tfd->next_pos_mcu;

        // calcul du nombre de pixel RGB à écrire dans le fichier en fonction de row_size
        uint32_t nb_write_h, nb_write_v;
        if (((tfd->next_pos_mcu%tfd->row_size) + nb_blocks_h*BLOCK_DIM*3 > tfd->row_size)){
                nb_write_h = (nb_blocks_h*BLOCK_DIM*3-((tfd->next_pos_mcu%tfd->row_size) + nb_blocks_h*BLOCK_DIM*3-tfd->row_size))/3;
                tfd->next_pos_mcu += 3*nb_write_h+(nb_blocks_v*BLOCK_DIM-1)*tfd->row_size;
        } else { 
                nb_write_h = nb_blocks_h*BLOCK_DIM;
                tfd->next_pos_mcu += 3*nb_write_h;
        }

        nb_write_v = ((nb_blocks_v*BLOCK_DIM) > (tfd->strip_bytes[tfd->current_line] / tfd->row_size)?(tfd->strip_bytes[tfd->current_line] / tfd->row_size) : (nb_blocks_v*BLOCK_DIM));
        

        
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
                fseek(tfd->file, current_position, 0);
                fwrite(rgb_mcu_row, sizeof(uint8_t), 3*nb_write_h, tfd->file);
                current_position += tfd->row_size;
        }
        
        //tfd->next_pos_mcu += nb_rgb_to_write;
        
        SAFE_FREE(rgb_mcu_row);
}

