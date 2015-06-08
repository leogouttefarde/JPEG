
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


struct tiff_file_desc {
        FILE *file;
        bool is_le;
        uint32_t width;
        uint32_t height;
        uint32_t rows_per_strip;
        uint32_t nb_strips;
        uint32_t next_pos_mcu;
        uint32_t current_line;
        uint32_t line_size;
        uint32_t row_size;
        uint8_t *write_buf;
};


static void write_short(struct tiff_file_desc *tfd, uint16_t value);

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

        if (tfd == NULL)
                return NULL;

        tfd->row_size = width * 3;
        tfd->write_buf = malloc(tfd->row_size);

        if (tfd->write_buf == NULL)
                return NULL;


        file = fopen(file_name, "wb");
        if (file == NULL)
                return NULL;

        tfd->file = file;

        tfd->is_le = true;
        tfd->width = width;
        tfd->height = height;
        tfd->rows_per_strip = row_per_strip;

        tfd->nb_strips = tfd->height / tfd->rows_per_strip;

        if (tfd->height % tfd->rows_per_strip)
                tfd->nb_strips++;


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
        uint32_t line_offset = strips_pos + 8 * tfd->nb_strips;
        uint32_t line_size = tfd->rows_per_strip * tfd->width * 3;

        if (tfd->nb_strips > 1)
                write_long(tfd, strips_pos);

        else
                write_long(tfd, line_offset);


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

        if (tfd->nb_strips > 1)
                write_long(tfd, strips_pos + 4 * tfd->nb_strips);

        else
                write_long(tfd, line_size);


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
        tfd->current_line = line_offset;
        tfd->line_size = line_size;

        /* Initialize the first MCU position */
        tfd->next_pos_mcu = 0;

        /* If there are multiple strips */
        if (tfd->nb_strips > 1) {

                /* Write all StripOffsets */
                for (uint32_t i = 0; i < tfd->nb_strips; i ++){
                        write_long(tfd, line_offset);
                        line_offset += line_size;
                }

                /* StripByteCounts data */
                for (uint32_t i = 0; i < tfd->nb_strips - 1; i++)
                        write_long(tfd, line_size);
        }


        /* Last line's height */
        uint32_t line_height = tfd->height % tfd->rows_per_strip;

        if (tfd->height > 0  && line_height == 0)
                line_height = tfd->rows_per_strip;

        line_size = line_height * tfd->width*3;

        write_long(tfd, line_size);


        return tfd;
}


static void write_short(struct tiff_file_desc *tfd, uint16_t value)
{
        if (tfd != NULL && tfd->file != NULL) {
                uint8_t buf[2];

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

static void write_long(struct tiff_file_desc *tfd, uint32_t value)
{
        if (tfd != NULL && tfd->file != NULL) {
                uint8_t buf[4];

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


        uint32_t nb_pixels;
        uint32_t pixel, index, k;
        uint32_t current_position;
        uint8_t *buf = tfd->write_buf;

        /* Skip to the next line if there's no space for this MCU */
        if (tfd->next_pos_mcu + (nb_blocks_v * BLOCK_DIM - 1) * tfd->row_size >= tfd->line_size) {
                tfd->current_line += tfd->line_size;
                tfd->next_pos_mcu = 0;
        }

        current_position = tfd->current_line + tfd->next_pos_mcu;


        /* Compute how many RGB pixels must be written depending on row_size */
        if (tfd->next_pos_mcu + nb_blocks_h * BLOCK_DIM * 3 > tfd->row_size)
                nb_pixels = tfd->row_size - tfd->next_pos_mcu;

        else
                nb_pixels = nb_blocks_h * BLOCK_DIM * 3;



        /* Write the MCU */
        for(uint32_t i = 0; i < nb_blocks_v*BLOCK_DIM; i++) {

                k = 0;
                fseek(tfd->file, current_position, SEEK_SET);

                for(uint32_t j = 0; j < nb_pixels/3; j++) {

                                index = i * nb_blocks_h * BLOCK_DIM + j;

                                pixel = mcu_rgb[index];

                                buf[k] = RED(pixel);
                                buf[k + 1] = GREEN(pixel);
                                buf[k + 2] = BLUE(pixel);

                                k += 3;
                }

                fwrite(buf, 1, nb_pixels, tfd->file);
                current_position += tfd->row_size;
        }

        tfd->next_pos_mcu += nb_pixels;
}

