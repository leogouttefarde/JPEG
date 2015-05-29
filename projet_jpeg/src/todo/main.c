
#include "common.h"
#include "bitstream.h"
#include "huffman.h"


bool read_short_BE(struct bitstream *stream, uint16_t *value)
{
        bool error = true;

        if (stream != NULL && value != NULL) {
                uint8_t count;
                uint32_t dest;

                count = read_bitstream(stream, 16, &dest, false);

                if (count == 16) {
                        error = false;
                        *value = (uint16_t)dest;
                }
        }

        return error;
}

bool read_byte(struct bitstream *stream, uint8_t *value)
{
        bool error = true;

        if (stream != NULL && value != NULL) {
                uint8_t count;
                uint32_t dest;

                count = read_bitstream(stream, 8, &dest, false);

                if (count == 8) {
                        error = false;
                        *value = (uint8_t)dest;
                }
        }

        return error;
}


int main(int argc, char **argv)
{
        if (argc > 1) {
                char *path = argv[1];
                struct bitstream *stream = create_bitstream(path);

                if (stream != NULL) {

                        // uint8_t count;
                        uint8_t byte, id;
                        uint16_t mark, size;
                        int32_t unread;
                        bool error = false;

                        /* Read section size */
                        error |= read_short_BE(stream, &mark);

                        /* Check for the SOI existence */
                        if (mark == 0xFFD8) {


                                struct comp_list {
                                        uint8_t i_c;
                                        uint8_t h_dc;
                                        uint8_t h_ac;
                                        struct comp_list *suiv;
                                };

                                struct comp_list comp_list;

                                struct color_comp {
                                        uint8_t nb_blocks_h;
                                        uint8_t nb_blocks_v;
                                        uint8_t i_q;
                                };

                                struct color_comp comps[0x100];


                                uint16_t height, width;
                                struct huff_table *huff_tables[2][4];
                                uint8_t quantif_tables[0x10][BLOCK_SIZE];

                                memset(&comp_list, 0, sizeof(comp_list));
                                memset(comps, 0, sizeof(comps));
                                memset(huff_tables, 0, sizeof(huff_tables));
                                memset(quantif_tables, 0, sizeof(quantif_tables));


                                do {
                                        /* Check the section 0xFF first value */
                                        read_byte(stream, &byte);
                                        // assert(byte == 0xFF);

                                        if (byte != 0xFF)
                                                error = true;

                                        /* Retrieve the section value */
                                        read_byte(stream, &id);


                                        /* Read section size */
                                        error |= read_short_BE(stream, &size);
                                        // printf("size = %04lX\n", size);
                                        unread = size - sizeof(size);



                                        /* Process each section */
                                        switch (id) {

                                        /* APP0 */
                                        case 0xE0:
                                                {
                                                char jfif[5];

                                                for (uint8_t i = 0; i < sizeof(jfif); i++)
                                                        read_byte(stream, (uint8_t*)&jfif[i]);

                                                /* JFIF header check */
                                                if (strcmp(jfif, "JFIF"))
                                                        error = true;
                                                }

                                                /* Skip other header data */
                                                skip_bitstream_until(stream, 0xFF);
                                                break;

                                        /* COM */
                                        case 0xFE:

                                                /* Skip the comment section */
                                                skip_bitstream_until(stream, 0xFF);
                                                break;

                                        /* DQT */
                                        case 0xDB:
                                                read_byte(stream, &byte);
                                                unread--;

                                                /* Read all quantification tables */
                                                do {
                                                        bool accuracy = ((uint8_t)byte) >> 4;

                                                        if (accuracy)
                                                                error = true;

                                                        else {
                                                                uint8_t i_q = byte & 0xF;
                                                                uint8_t *quantif_table = (uint8_t*)&quantif_tables[i_q];

                                                                // printf("unread = %d\n", unread);
                                                                for (uint8_t i = 0; i < BLOCK_SIZE; i++) {
                                                                        error |= read_byte(stream, &quantif_table[i]);
                                                                        unread--;
                                                                }

                                                                // printf("unread = %d\n", unread);
                                                        }

                                                } while (unread > 0 && !error);

                                                skip_bitstream_until(stream, 0xFF);
                                                break;

                                        /* SOF0 */
                                        case 0xC0:
                                                {
                                                        uint8_t accuracy;
                                                        read_byte(stream, &accuracy);

                                                        if (accuracy != 8) {
                                                                printf("ERROR : this baseline JPEG decoder only supports 8 bits accuracy\n");

                                                                error = true;
                                                        }

                                                        uint8_t nb_comps;


                                                        read_short_BE(stream, &height);
                                                        read_short_BE(stream, &width);

                                                        read_byte(stream, &nb_comps);


                                                        if (nb_comps != 3) {
                                                                printf("ERROR : this baseline JPEG decoder only supports 3 component images\n");

                                                                error = true;
                                                        }
                                                        else {
                                                                for (uint8_t i = 0; i < nb_comps; i++) {
                                                                        uint8_t i_c, i_q;
                                                                        uint8_t h_sampling_factor;
                                                                        uint8_t v_sampling_factor;

                                                                        read_byte(stream, &i_c);

                                                                        read_byte(stream, &byte);
                                                                        h_sampling_factor = byte >> 4;
                                                                        v_sampling_factor = byte & 0xF;

                                                                        read_byte(stream, &i_q);

                                                                        comps[i_c].nb_blocks_h = h_sampling_factor;
                                                                        comps[i_c].nb_blocks_v = v_sampling_factor;
                                                                        comps[i_c].i_q = i_q;
                                                                }
                                                        }
                                                }

                                                skip_bitstream_until(stream, 0xFF);
                                                break;

                                        /* DHT */
                                        case 0xC4:
                                                /* Read all Huffman tables */
                                                while (unread > 0 && !error) {
                                                        uint8_t unused, type, index;

                                                        error |= read_byte(stream, &byte);
                                                        unread--;

                                                        unused = byte >> 5;

                                                        // 0 => DC, 1 => AC
                                                        type = (byte >> 4) & 1;

                                                        index = byte & 0xF;


                                                        // unused toujours à 0, sinon erreur
                                                        // Jamais plus de 4 tables AC ou DC, sinon erreur
                                                        if (unused || index > 3 || type > 1)
                                                                error = true;


                                                        uint16_t nb_byte_read;
                                                        struct huff_table *table;

                                                        table = load_huffman_table(stream, &nb_byte_read);

                                                        if (nb_byte_read == -1 || table == NULL)
                                                                error = true;

                                                        if (!error)
                                                                huff_tables[type][index] = table;

                                                        unread -= nb_byte_read;
                                                }

                                                skip_bitstream_until(stream, 0xFF);
                                                break;

                                        /* SOS : Start Of Scan */
                                        case 0xDA:
                                                {
                                                        uint8_t nb_comps;
                                                        read_byte(stream, &nb_comps);


                                                        struct comp_list *c_comp = &comp_list, *prev = NULL;

                                                        for (uint8_t i = 0; i < nb_comps; i++) {

                                                                if (prev)
                                                                        c_comp = calloc(1, sizeof(struct comp_list));

                                                                assert(c_comp);


                                                                read_byte(stream, &byte);
                                                                c_comp->i_c = byte;
                                                                // printf("i_c = %i\n", byte);

                                                                read_byte(stream, &byte);
                                                                c_comp->h_dc = byte >> 4;
                                                                c_comp->h_ac = byte & 0xF;

                                                                if (prev)
                                                                        prev->suiv = c_comp;

                                                                prev = c_comp;
                                                        }

                                                        read_byte(stream, &byte);
                                                        read_byte(stream, &byte);
                                                        read_byte(stream, &byte);
                                                }

                                                /* Lecture flux principal */
                                                struct comp_list *cur_comp = &comp_list;

                                                // Calcul du nombre de MCU à faire
                                                // (+ reconstitution de l'image)
                                                while (cur_comp) {

                                                        // comps[cur_comp->i_c].nb_blocks_h
                                                        // comps[cur_comp->i_c].nb_blocks_v
                                                        // comps[cur_comp->i_c].i_q

                                                        // cur_comp->h_dc
                                                        // cur_comp->h_ac
                                                        uint8_t nb = comps[cur_comp->i_c].nb_blocks_h * comps[cur_comp->i_c].nb_blocks_v;

                                                        // printf("nb = %d\n", nb);
                                                        /* Y */
                                                        for (uint8_t n = 0; n < nb; n++)
                                                                for (uint8_t i = 0; i < BLOCK_SIZE; i++)
                                                                        read_byte(stream, &byte);

                                                        /* Cb */
                                                        for (uint8_t i = 0; i < BLOCK_SIZE; i++)
                                                                read_byte(stream, &byte);

                                                        /* Cr */
                                                        for (uint8_t i = 0; i < BLOCK_SIZE; i++)
                                                                read_byte(stream, &byte);


                                                        cur_comp = cur_comp->suiv;
                                                }


                                                printf("Section SOS à finir\n");
                                                error = 1;
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
                                        // printf("end_of_bitstream(stream) : %d\n", end_of_bitstream(stream));
                                        // printf("error : %d\n", error);

                                } while (!end_of_bitstream(stream) && !error);


                                for (uint8_t i = 0; i < 2; i++)
                                        for (uint8_t j = 0; j < 4; j++)
                                                free_huffman_table(huff_tables[i][j]);
                        }


                        free_bitstream(stream);
                }
        }

        return EXIT_SUCCESS;
}



