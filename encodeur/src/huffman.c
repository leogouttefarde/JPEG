
#include "huffman.h"
#include "common.h"
#include "priority_queue.h"


struct node {
        struct huff_table *left;
        struct huff_table *right;
};

enum node_type {
        NODE,
        LEAF,
        NB_NODE_TYPES
};

struct huff_table {
        enum node_type type;
        uint32_t code;
        uint8_t size;

        union {
                struct node node;
                int8_t val;
        } u;
};


struct huff_table* create_node(enum node_type type, uint32_t code,
                                uint8_t size, int8_t val)
{
        struct huff_table *node = malloc(sizeof(struct huff_table));

        if (node != NULL) {
                node->type = type;
                node->code = code;
                node->size = size;

                if (type == NODE) {
                        node->u.node.left = NULL;
                        node->u.node.right = NULL;
                }
                else
                        node->u.val = val;
        }

        return node;
}

int8_t add_huffman_code(int8_t value, uint8_t code_size, struct huff_table *parent)
{
        int8_t error = 0;


        if (parent == NULL || parent->type == LEAF)
                error = 1;

        else {
                uint32_t code = parent->code << 1;
                uint8_t size = parent->size + 1;

                struct huff_table **left, **right;

                left = &parent->u.node.left;
                right = &parent->u.node.right;


                if (code_size == 0) {
                        if (*left == NULL)
                                *left = create_node(LEAF, code, size, value);

                        else if (*right == NULL)
                                *right = create_node(LEAF, code + 1, size, value);

                        else
                                error = -1;
                }
                else {
                        if (*left == NULL)
                                *left = create_node(NODE, code, size, 0);

                        error = add_huffman_code(value, code_size - 1, *left);

                        if (error) {
                                if (*right == NULL)
                                        *right = create_node(NODE, code + 1, size, 0);

                                error = add_huffman_code(value, code_size - 1, *right);
                        }
                }
        }

        return error;
}

struct huff_table *load_huffman_table(
                struct bitstream *stream, uint16_t *nb_byte_read)
{
        uint8_t code_sizes[16];
        uint16_t nb_codes = 0;
        int32_t size_read = 0;
        uint32_t dest;
        struct huff_table *table = NULL;

        if (nb_byte_read == NULL)
                return NULL;

        *nb_byte_read = -1;


        for (uint8_t i = 0; i < 16; i++) {
                size_read += read_bitstream(stream, 8, &dest, false);
                code_sizes[i] = dest & 0xFF;
        }

        for (uint8_t i = 0; i < sizeof(code_sizes); ++i)
                nb_codes += code_sizes[i];


        /* There must be less than 256 different codes */
        if (nb_codes >= 256)
                return NULL;

        else {
                table = malloc(sizeof(struct huff_table));

                if (table == NULL)
                        return NULL;
        }



        memset(table, 0, sizeof(struct huff_table));

        for (uint8_t i = 0; i < sizeof(code_sizes); ++i) {
                for (uint8_t j = 0; j < code_sizes[i]; ++j) {
                        size_read += read_bitstream(stream, 8, &dest, false);
                        add_huffman_code(dest & 0xFF, i, table);
                }
        }

        *nb_byte_read = size_read / 8;


        return table;
}

int8_t next_huffman_value(struct huff_table *table, 
                struct bitstream *stream)
{
        int8_t bit;
        int8_t result = 0;
        uint32_t dest;

        while (table && table->type == NODE) {
                read_bitstream(stream, 1, &dest, true);
                bit = dest & 1;

                if (bit)
                        table = table->u.node.right;

                else
                        table = table->u.node.left;
        }

        if (table != NULL) {
                result = table->u.val;
        }

        return result;
}

void free_huffman_table(struct huff_table *table)
{
        if (table != NULL) {
                if (table->type == NODE) {
                        free_huffman_table(table->u.node.left);
                        free_huffman_table(table->u.node.right);
                }

                SAFE_FREE(table);
        }
}

struct huff_table *get_huffman_code(int8_t value, struct huff_table *table)
{
        struct huff_table *code = NULL;

        if (table != NULL) {
                if (table->type == NODE) {
                        code = get_huffman_code(value, table->u.node.left);

                        if (code == NULL)
                                code = get_huffman_code(value, table->u.node.right);

                } else if (table->type == LEAF && value == table->u.val)
                        code = table;
        }

        return code;
}

bool write_huffman_value(int8_t value, struct huff_table *table,
                         struct bitstream *stream)
{
        // printf("write_huffman_value\n");
        int8_t bit;
        bool success = false;

        struct huff_table *leaf = get_huffman_code(value, table);

        if (leaf != NULL) {
                uint32_t code = leaf->code;
                uint8_t size = leaf->size;

                for (uint8_t i = 0; i < size; i++) {
                        bit = (code >> (size - 1 - i)) & 1;
                        write_bit(stream, bit, true);
                }

                success = true;
        }
        else
                printf("FATAL ERROR : no Huffman code for %d !\n", value);


        return success;
}

struct huff_table *create_huffman_tree(uint32_t freqs[0x100])
{
        // printf("create_queue\n");
        struct priority_queue *queue = create_queue(0x100);
        struct huff_table *node = NULL;

        if (queue == NULL)
                return NULL;


        for (uint16_t val = 0; val < 0x100; val++) {
                if (freqs[val] > 0) {
                        node = create_node(LEAF, 0, 0, val);
                        insert_queue(queue, freqs[val], node);
                }
        }

        struct huff_table *tree = NULL;
        struct huff_table *child0 = NULL;
        struct huff_table *child1 = NULL;
        bool status = true;
        uint32_t p1, p2;


        /* Construction de l'arbre de Huffman */
        while (status) {
                // printf("status = %d\n", status);
                status = best_queue(queue, &p1, &child0);

                /* S'il y a un arbre de priorité minimale */
                if (status) {
                        delete_queue(queue);
                        status = best_queue(queue, &p2, &child1);
                }

                /* S'il y a un nouvel arbre de priorité minimale */
                if (status) {
                        delete_queue(queue);


                        node = create_node(NODE, -1, -1, -1);

                        if (node != NULL) {

                                child1->code = child1->code << 1 | 0;
                                child0->code = child0->code << 1 | 1;
                                child1->size++;
                                child0->size++;

                                node->u.node.left = child1;
                                node->u.node.right = child0;

                                /* On fusionne les deux arbres avant de les ajouter à la file */
                                insert_queue(queue, p1 + p2, node);
                        } else
                                break;
                }

                /*
                * Sinon, l'arbre de Huffman est
                * le dernier arbre extrait : Fils0
                */
                else
                        tree = child0;
        }



        return tree;
}

void write_huffman_table(struct bitstream *stream, struct huff_table *table)
{
        // uint8_t code_sizes[16];
        // uint16_t nb_codes = 0;
        // int32_t size_read = 0;
        // uint32_t dest;
        // struct huff_table *table = NULL;

        // if (nb_byte_read == NULL)
        //         return NULL;

        // *nb_byte_read = -1;


        // for (uint8_t i = 0; i < 16; i++) {
        //         size_read += read_bitstream(stream, 8, &dest, false);
        //         code_sizes[i] = dest & 0xFF;
        // }

        // for (uint8_t i = 0; i < sizeof(code_sizes); ++i)
        //         nb_codes += code_sizes[i];


        // /* There must be less than 256 different codes */
        // if (nb_codes >= 256)
        //         return NULL;

        // else {
        //         table = malloc(sizeof(struct huff_table));

        //         if (table == NULL)
        //                 return NULL;
        // }



        // memset(table, 0, sizeof(struct huff_table));

        // for (uint8_t i = 0; i < sizeof(code_sizes); ++i) {
        //         for (uint8_t j = 0; j < code_sizes[i]; ++j) {
        //                 size_read += read_bitstream(stream, 8, &dest, false);
        //                 add_huffman_code(dest & 0xFF, i, table);
        //         }
        // }

        // *nb_byte_read = size_read / 8;


        // return table;
}

