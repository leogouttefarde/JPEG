
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

#define bytes2long_be(b)    (GET_BYTE(b[0]) << 24 | GET_BYTE(b[1]) << 16 | GET_BYTE(b[2]) << 8 | GET_BYTE(b[3]))
#define bytes2long_le(b)    (GET_BYTE(b[3]) << 24 | GET_BYTE(b[2]) << 16 | GET_BYTE(b[1]) << 8 | GET_BYTE(b[0]))
#define bytes2long(b, le)   (le ? bytes2long_le(b) : bytes2long_be(b))

#define bytes2short_be(b)   (GET_BYTE(b[0]) << 8 | GET_BYTE(b[1]))
#define bytes2short_le(b)   (GET_BYTE(b[1]) << 8 | GET_BYTE(b[0]))
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
        uint32_t line_size;
        uint32_t row_size;
        uint32_t read_lines;
        uint8_t *write_buf;
};


/* Reads a short from the file */
static uint16_t read_short(struct tiff_file_desc *tfd, bool *error);

/* Reads a long from the file */
static uint32_t read_long(struct tiff_file_desc *tfd, bool *error);

/* Reads an IFD entry and updates the right tiff fields */
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


        /* Read all IFD entries */
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

/* Lit une ligne de l'image TIFF ouverte avec init_tiff_read.
 * Renvoie true si une erreur est survenue, false si pas d'erreur. */
bool read_tiff_line(struct tiff_file_desc *tfd, uint32_t *line_rgb)
{
        size_t ret;
        bool error = false;
        char buf[4];
        uint32_t i = 0;
        uint32_t *cur_line = &tfd->current_line;
        uint32_t *offsets = tfd->strip_offsets;

        memset(buf, 0, sizeof(buf));


        /*
         * If rows_per_strip is zero,
         * there are no multiple strips.
         * Else go to the next strip when required.
         */
        if (tfd->rows_per_strip > 0)
        if (tfd->read_lines >= tfd->rows_per_strip) {

                /* If a next strip exists */
                if (++*cur_line < tfd->nb_strips) {
                        fseek(tfd->file, offsets[*cur_line], SEEK_SET);
                        tfd->read_lines = 0;
                }
        }

        /* Read a whole line */
        for (uint32_t w = 0; w < tfd->width; w++) {

                /* Read each pixel */
                ret = fread(buf, 1, tfd->samples_per_pixels, tfd->file);

                /* Error handling */
                if (ret != tfd->samples_per_pixels) {
                        error = true;
                        break;
                }

                if (tfd->samples_per_pixels == 1)
                        buf[1] = buf[2] = buf[0];

                /* Error handling */
                line_rgb[i++] = GET_BYTE(buf[0]) << 16 | GET_BYTE(buf[1]) << 8 | GET_BYTE(buf[2]);
        }

        /* Increment the line counter */
        tfd->read_lines++;


        return error;
}

/* Reads a short from the file */
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

/* Reads a long from the file */
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

/* Reads an IFD entry and updates the right tiff fields */
static void read_ifd_entry(struct tiff_file_desc *tfd, bool *error)
{
        if (error == NULL || *error)
                return;


        uint16_t tag;
        uint16_t type;
        uint32_t count;
        uint32_t offset;
        uint32_t value;
        uint32_t next_value = 0;
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


        offset = bytes2long(buf, tfd->is_le);


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
                value = offset;
                break;

        /* Unused values */
        case ASCII:
        case RATIONAL:
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
                        fseek(tfd->file, offset, SEEK_SET);

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
                        fseek(tfd->file, offset, SEEK_SET);

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
                        fseek(tfd->file, offset, SEEK_SET);

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
                SAFE_FREE(tfd->write_buf);
        }

        SAFE_FREE(tfd);
}



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

