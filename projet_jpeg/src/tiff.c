
#include "tiff.h"
#include "common.h"


/* TIFF constants */
#define BYTE              0x0001
#define ASCII             0x0002
#define SHORT             0x0003
#define LONG              0x0004
#define RATIONAL          0x0005

#define ZERO              0x0000
#define UN                0x0001
#define DEUX              0x0002
#define TROIS             0x0003

/* Endianness */
#define LITTLE_ENDIAN     0x4949
#define BIG_ENDIAN        0x4D4D

/* IFD tags */
#define IMAGE_WIDTH       0x0100
#define IMAGE_LENGTH      0x0101
#define BITS_PER_SAMPLE   0x0102
#define COMPRESSION       0x0103
#define PHOTOMETRIC       0x0106
#define STRIP_OFFSETS     0x0111
#define SAMPLES_PER_PIXEL 0x0115
#define ROWS_PER_STRIP    0x0116
#define STRIP_BYTE_COUNTS 0x0117
#define X_RESOLUTION      0x011A
#define Y_RESOLUTION      0x011B
#define RESOLUTION_UNIT   0x0128
#define SOFTWARE          0x0131


/* Macros */
#define _BYTE(c, i)       ((c >> 8*i) & 0xFF)
#define RED(c)            _BYTE(c, 2)
#define GREEN(c)          _BYTE(c, 1)
#define BLUE(c)           _BYTE(c, 0)


/*
 * Internal TIFF structure informations
 */
struct tiff_file_desc {

        /* This tiff file's pointer */
        FILE *file;

        /* Indicates if this tiff uses little endian or not */
        bool is_le;

        /* Dimensions */
        uint32_t width;
        uint32_t height;

        /* Number of rows per strip */
        uint32_t rows_per_strip;

        /* Strip informations */
        uint32_t nb_strips;
        uint32_t *strip_offsets;
        uint32_t *strip_bytes;

        /* Internal informations */
        uint32_t next_pos_mcu;
        uint32_t current_line;
        uint32_t line_size;
        uint32_t row_size;
        uint8_t *write_buf;
};


/* Writes a short in the file */
static void write_short(struct tiff_file_desc *tfd, uint16_t value);

/* Writes a long in the file */
static void write_long(struct tiff_file_desc *tfd, uint32_t value);


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
        FILE *file = NULL;
        struct tiff_file_desc *tfd = calloc(1, sizeof(struct tiff_file_desc));
        uint32_t line_size;

        if (tfd == NULL)
                return NULL;


        /* Allocate & check write_buf */
        tfd->row_size = width * 3;

        tfd->write_buf = malloc(tfd->row_size);
        if (tfd->write_buf == NULL)
                return NULL;


        /* Allocate & check strip_offsets */
        tfd->nb_strips = height / row_per_strip;

        if (height % row_per_strip)
                tfd->nb_strips++;

        tfd->strip_offsets = malloc(tfd->nb_strips * sizeof(uint32_t));
        if (tfd->strip_offsets == NULL)
                return NULL;


        /* Allocate & check strip_bytes */
        tfd->strip_bytes = malloc(tfd->nb_strips * sizeof(uint32_t));
        if (tfd->strip_bytes == NULL)
                return NULL;


        file = fopen(file_name, "wb");
        if (file == NULL)
                return NULL;

        tfd->file = file;

        tfd->is_le = true;
        tfd->width = width;
        tfd->height = height;
        tfd->rows_per_strip = row_per_strip;


        /* Software comment definition */
        const char *comment = COMMENT;
        const uint32_t comment_size = strlen(comment) + 1;


        /* Header construction */
        const uint32_t ifd_offset = 8 + comment_size;
        const uint16_t entry_count = 13;

        /* Endianness + TIFF identification */
        write_short(tfd, LITTLE_ENDIAN);
        write_short(tfd, 42);

        /* IFD offset */
        write_long(tfd, ifd_offset);

        /* Software comment */
        fwrite(comment, 1, comment_size, file);


        uint32_t next = ifd_offset + 2 + 12 * entry_count + 4;

        /* IFD data */
        write_short(tfd, entry_count);

        /* Image Width */
        write_short(tfd, IMAGE_WIDTH);
        write_short(tfd, LONG);
        write_long(tfd, 1);
        write_long(tfd, tfd->width);

        /* Image Length */
        write_short(tfd, IMAGE_LENGTH);
        write_short(tfd, LONG);
        write_long(tfd, 1);
        write_long(tfd, tfd->height);


        /* BitsPerSample */
        write_short(tfd, BITS_PER_SAMPLE);
        write_short(tfd, SHORT);
        write_long(tfd, 3);
        write_long(tfd, next);
        next += 3 * 2;

        /* Compression */
        write_short(tfd, COMPRESSION);
        write_short(tfd, SHORT);
        write_long(tfd, 1);
        write_long(tfd, 1);

        /* PhotometricInterpretation */
        write_short(tfd, PHOTOMETRIC);
        write_short(tfd, SHORT);
        write_long(tfd, 1);
        write_long(tfd, 2);

        /* StripOffsets */
        write_short(tfd, STRIP_OFFSETS);
        write_short(tfd, LONG);
        write_long(tfd, tfd->nb_strips);

        const uint32_t strips_pos = next + 16;
        uint32_t line_offset = strips_pos;


        /* One row handling */
        if (tfd->nb_strips > 1) {
                line_offset += 8 * tfd->nb_strips;
                write_long(tfd, strips_pos);
        }

        else {
                write_long(tfd, line_offset);
                tfd->strip_offsets[0] = line_offset;
        }


        /* SamplesPerPixel */
        write_short(tfd, SAMPLES_PER_PIXEL);
        write_short(tfd, SHORT);
        write_long(tfd, 1);
        write_long(tfd, 3);

        /* RowsPerStrip */
        write_short(tfd, ROWS_PER_STRIP);
        write_short(tfd, LONG);
        write_long(tfd, 1);
        write_long(tfd, tfd->rows_per_strip);

        /* StripByteCounts */
        write_short(tfd, STRIP_BYTE_COUNTS);
        write_short(tfd, LONG);
        write_long(tfd, tfd->nb_strips);


        /* Last line's height */
        uint32_t line_height = tfd->height % tfd->rows_per_strip;

        if (tfd->height > 0  && line_height == 0)
                line_height = tfd->rows_per_strip;


        /* One row handling */
        if (tfd->nb_strips > 1) {
                line_size = tfd->rows_per_strip * tfd->width * 3;
                write_long(tfd, strips_pos + 4 * tfd->nb_strips);
        }

        else {
                line_size = line_height * tfd->width * 3;
                write_long(tfd, line_size);

                tfd->strip_bytes[0] = line_size;
        }


        /* XResolution */
        write_short(tfd, X_RESOLUTION);
        write_short(tfd, RATIONAL);
        write_long(tfd, 1);
        write_long(tfd, next);
        next += 2 * 4;

        /* YResolution */
        write_short(tfd, Y_RESOLUTION);
        write_short(tfd, RATIONAL);
        write_long(tfd, 1);
        write_long(tfd, next);
        next += 2 * 4;

        /* ResolutionUnit */
        write_short(tfd, RESOLUTION_UNIT);
        write_short(tfd, SHORT);
        write_long(tfd, 1);
        write_long(tfd, 2);

        /* Software */
        write_short(tfd, SOFTWARE);
        write_short(tfd, ASCII);
        write_long(tfd, comment_size);
        write_long(tfd, 8);


        /* No other IFD */
        write_long(tfd, 0);


        /* BitsPerSample data */
        write_short(tfd, 8);
        write_short(tfd, 8);
        write_short(tfd, 8);

        /* XResolution data */
        write_long(tfd, 100);
        write_long(tfd, 1);

        /* YResolution data */
        write_long(tfd, 100);
        write_long(tfd, 1);


        /* Initialize internal writing data */
        tfd->current_line = 0;
        tfd->line_size = line_size;

        /* Initialize the first MCU position */
        tfd->next_pos_mcu = 0;

        /* If there are multiple strips */
        if (tfd->nb_strips > 1) {

                /* Write all StripOffsets */
                for (uint32_t i = 0; i < tfd->nb_strips; i++){
                        write_long(tfd, line_offset);
                        tfd->strip_offsets[i] = line_offset;

                        line_offset += line_size;
                }


                /* StripByteCounts data */
                for (uint32_t i = 0; i < tfd->nb_strips - 1; i++) {
                        write_long(tfd, line_size);
                        tfd->strip_bytes[i] = line_size;
                }

                /* Last line's size */
                line_size = line_height * tfd->width*3;
                tfd->strip_bytes[tfd->nb_strips-1] = line_size;

                write_long(tfd, line_size);
        }


        return tfd;
}


/* Writes a short in the file */
static void write_short(struct tiff_file_desc *tfd, uint16_t value)
{
        if (tfd != NULL && tfd->file != NULL) {
                uint8_t buf[2];

                /* Endianness management */
                if (tfd->is_le) {
                        buf[0] = value;
                        buf[1] = value >> 8;
                } else {
                        buf[0] = value >> 8;
                        buf[1] = value;
                }

                fwrite(buf, 1, sizeof(buf), tfd->file);
        }
}

/* Writes a long in the file */
static void write_long(struct tiff_file_desc *tfd, uint32_t value)
{
        if (tfd != NULL && tfd->file != NULL) {
                uint8_t buf[4];

                /* Endianness management */
                if (tfd->is_le) {
                        buf[0] = value;
                        buf[1] = value >> 8;
                        buf[2] = value >> 16;
                        buf[3] = value >> 24;
                } else {
                        buf[0] = value >> 24;
                        buf[1] = value >> 16;
                        buf[2] = value >> 8;
                        buf[3] = value;
                }

                fwrite(buf, 1, sizeof(buf), tfd->file);
        }
}

/* Ferme le fichier associé à la structure tiff_file_desc passée en
 * paramètre et désalloue la mémoire occupée par cette structure. */
void close_tiff_file(struct tiff_file_desc *tfd)
{
        if (tfd != NULL) {
                if (tfd->file != NULL)
                        fclose(tfd->file);   

                SAFE_FREE(tfd->write_buf);
                SAFE_FREE(tfd->strip_offsets);
                SAFE_FREE(tfd->strip_bytes);
        }

        SAFE_FREE(tfd);
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
        if (tfd == NULL && tfd->file == NULL)
                return;


        uint8_t *buf = tfd->write_buf;
        uint32_t pixel, index, k;
        uint32_t current_position;
        uint32_t nb_write_h, nb_write_v;


        /* Compute required values */
        const uint32_t row_size = tfd->row_size;
        const uint32_t *cur_line = &tfd->current_line;
        const uint32_t *offsets = tfd->strip_offsets;
        const uint32_t cur_offset = offsets[*cur_line];
        const uint32_t block_size_to_add = (nb_blocks_v * BLOCK_DIM - 1) * row_size;
        const uint32_t new_offset = cur_offset + tfd->next_pos_mcu + block_size_to_add;

        /* Skip to the next line if there's no space for this MCU */
        if (*cur_line < (tfd->nb_strips - 1) && new_offset >= offsets[*cur_line+1]) {
                tfd->current_line++;
                tfd->next_pos_mcu = 0;
        }

        current_position = offsets[*cur_line] + tfd->next_pos_mcu;



        const uint32_t h_block_size = nb_blocks_h * BLOCK_DIM * 3;
        const uint32_t size_written = tfd->next_pos_mcu % row_size;

        /* Compute how many RGB pixels must be written depending on row_size */
        if (size_written + h_block_size > row_size) {
                nb_write_h = (row_size - size_written) / 3;
                tfd->next_pos_mcu += block_size_to_add;
        }
        else
                nb_write_h = nb_blocks_h*BLOCK_DIM;

        tfd->next_pos_mcu += 3 * nb_write_h;



        const uint32_t strip_size = tfd->strip_bytes[*cur_line];
        const uint32_t v_blocks_length = nb_blocks_v * BLOCK_DIM;
        const uint32_t v_blocks_max = strip_size / row_size;

        if (v_blocks_length > v_blocks_max)
                nb_write_v = v_blocks_max;

        else
                nb_write_v = v_blocks_length;


        /* Write the MCU */
        for(uint32_t i = 0; i < nb_write_v; i++) {

                k = 0;
                fseek(tfd->file, current_position, SEEK_SET);

                for(uint32_t j = 0; j < nb_write_h; j++) {

                                index = i * nb_blocks_h * BLOCK_DIM + j;
                                pixel = mcu_rgb[index];

                                buf[k++] = RED(pixel);
                                buf[k++] = GREEN(pixel);
                                buf[k++] = BLUE(pixel);
                }

                fwrite(buf, 1, 3 * nb_write_h, tfd->file);
                current_position += row_size;
        }
}

