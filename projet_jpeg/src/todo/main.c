
#include "common.h"


bool read_short_BE(FILE *file, uint16_t *value)
{
        bool error = true;

        if (file != NULL && value != NULL) {
                uint32_t count;
                char buf[2];

                count = fread(buf, sizeof(buf), 1, file);

                if (count > 0) {
                        error = false;
                        *value = (buf[0] & 0xFF) << 8 | (buf[1] & 0xFF);
                }
        }

        return error;
}


int main(int argc, char **argv)
{
        if (argc > 1) {
                FILE *file = NULL;
                char *path = argv[1];

                file = fopen(path, "rb");

                if (file != NULL) {

                        uint16_t mark, size, unread;
                        int byte, id;
                        bool error = false;

                        /* Read section size */
                        error |= read_short_BE(file, &mark);

                        /* On vérifie la présence du marqueur SOI */
                        if (mark == 0xFFD8) {

                                do {
                                        /* Check the section 0xFF first value */
                                        byte = fgetc(file);
                                        // assert(byte == 0xFF);

                                        if (byte != 0xFF || byte == EOF)
                                                error = true;

                                        /* Retrieve the section value */
                                        id = fgetc(file);


                                        /* Read section size */
                                        error |= read_short_BE(file, &size);
                                        unread = size - sizeof(size);


                                        char jfif[5];
                                        uint16_t nb_tables;


                                        /* Process each section */
                                        switch (id) {

                                        /* APP0 */
                                        case 0xE0:
                                                // char jfif[5];
                                                unread -= fread(jfif, 1, sizeof(jfif), file);

                                                /* JFIF header check */
                                                if (strcmp(jfif, "JFIF"))
                                                        error = true;

                                                /* Skip other header data */
                                                fseek(file, unread, SEEK_CUR);
                                                break;

                                        /* COM */
                                        case 0xFE:
                                                /* Skip the comment section */
                                                fseek(file, unread, SEEK_CUR);
                                                break;

                                        /* DQT */
                                        case 0xDB:
                                                byte = fgetc(file);

                                                if (byte != EOF) {
                                                        unread--;
                                                        uint16_t read;

                                                        do {
                                                                bool accuracy = ((uint8_t)byte) >> 4;

                                                                uint8_t i_q = byte & 0xFF;

                                                                uint8_t quantif_size = accuracy ? 2 : 1;
                                                                uint8_t quantif_table[quantif_size * BLOCK_SIZE];

                                                                printf("unread = %d\n", unread);
                                                                uint16_t read = fread(quantif_table, BLOCK_SIZE, quantif_size, file);
                                                                unread -= read * BLOCK_SIZE;

                                                                printf("read = %d\n", read);
                                                                printf("unread = %d\n", unread);

                                                        } while (unread > 0 && read > 0);
                                                }
                                                break;

                                        /* SOF0 */
                                        case 0xC0:
                                                fseek(file, unread, SEEK_CUR);
                                                break;

                                        /* DHT */
                                        case 0xC4:
                                                fseek(file, unread, SEEK_CUR);
                                                break;

                                        /* SOS */
                                        case 0xDA:
                                                fseek(file, unread, SEEK_CUR);

                                                /* Lecture flux principal */
                                                fseek(file, -2, SEEK_END);
                                                break;

                                        /* EOI */
                                        case 0xD9:
                                                /* Fin de l'image */
                                                break;

                                        default:
                                                printf("Section non supportée :\n0x%02X\n", id);
                                                error = true;
                                        }

                                        printf("Section : 0x%02X\n", id);
                                        // printf("feof(file) : %d\n", feof(file));
                                        // printf("error : %d\n", error);

                                } while (!feof(file) && !error);
                        }


                        fclose(file);
                }
        }

        return EXIT_SUCCESS;
}



