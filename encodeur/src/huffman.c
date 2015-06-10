
#include "huffman.h"
#include "common.h"
#include "priority_queue.h"


/*
 * Internal node structure
 */
struct node {
        struct huff_table *left;
        struct huff_table *right;
};

/*
 * Node type :
 * leaf or internal node
 */
enum node_type {
        NODE,
        LEAF,
        NB_NODE_TYPES
};

/*
 * Huffman table node structure
 */
struct huff_table {

        /* Node type : NODE or LEAF */
        enum node_type type;

        /* Node's Huffman code */
        uint32_t code;

        /* Node's code size */
        uint8_t size;

        union {
                struct node node;
                int8_t val;
        } u;
};


/*
 * Creates a Huffman table node
 * according to the "type" argument.
 */
static struct huff_table* create_node(enum node_type type, uint32_t code,
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

/*
 * Recursively adds a Huffman value to a Huffman tree.
 */
static int8_t add_huffman_code(int8_t value, uint8_t code_size, struct huff_table *parent)
{
        int8_t error = 0;


        /*
         * If we attempt to insert
         * a value on a leaf or a NULL pointer,
         * return an error.
         */
        if (parent == NULL || parent->type == LEAF)
                error = 1;

        else {
                uint32_t code = parent->code << 1;
                uint8_t size = parent->size + 1;

                struct huff_table **left, **right;

                left = &parent->u.node.left;
                right = &parent->u.node.right;


                /*
                 * Once the value to insert is deep enough,
                 * add it as a new leaf.
                 */
                if (code_size == 0) {
                        if (*left == NULL)
                                *left = create_node(LEAF, code, size, value);

                        else if (*right == NULL)
                                *right = create_node(LEAF, code + 1, size, value);

                        else
                                error = -1;
                }

                /*
                 * Else recursively add it deeper and
                 * create non-existant internal nodes
                 */
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

/*
 * Loads a Huffman table from the input stream.
 */
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


        /* Read the number of symbols per size */
        for (uint8_t i = 0; i < sizeof(code_sizes); i++) {
                size_read += read_bitstream(stream, 8, &dest, false);
                code_sizes[i] = dest & 0xFF;
                nb_codes += code_sizes[i];
        }


        /* There must be less than 256 different codes */
        if (nb_codes > 256)
                return NULL;

        else {
                /* Allocate the Huffman tree */
                table = calloc(1, sizeof(struct huff_table));
                if (table == NULL)
                        return NULL;
        }

        /*
         * Read all values and add them to the Huffman tree,
         * with their corresponding size.
         */
        for (uint8_t i = 0; i < sizeof(code_sizes); ++i) {
                for (uint8_t j = 0; j < code_sizes[i]; ++j) {
                        size_read += read_bitstream(stream, 8, &dest, false);
                        add_huffman_code(dest & 0xFF, i, table);
                }
        }

        *nb_byte_read = size_read / 8;


        return table;
}

/*
 * Reads a Huffman value from the input stream.
 */
int8_t next_huffman_value(struct huff_table *table, 
                struct bitstream *stream)
{
        int8_t bit;
        int8_t result = 0;
        uint32_t dest;

        /* Advance in the Huffman tree
         * according to each read stream bit
         * until a leaf is found */
        while (table && table->type == NODE) {
                read_bitstream(stream, 1, &dest, true);
                bit = dest & 1;

                /* Go right if it's a 1 */
                if (bit)
                        table = table->u.node.right;

                /* Go left if it's a 0 */
                else
                        table = table->u.node.left;
        }

        if (table != NULL)
                result = table->u.val;

        return result;
}

/*
 * Recursively frees a Huffman table.
 */
void free_huffman_table(struct huff_table *table)
{
        if (table != NULL) {

                /* If it's an internal node, free it recursively */
                if (table->type == NODE) {
                        free_huffman_table(table->u.node.left);
                        free_huffman_table(table->u.node.right);
                }

                SAFE_FREE(table);
        }
}

/*
 * Recursively finds the Huffman code for a given value.
 */
static struct huff_table *get_huffman_code(int8_t value, struct huff_table *table)
{
        struct huff_table *code = NULL;

        if (table != NULL) {

                /* Recursively continue on internal nodes */
                if (table->type == NODE) {
                        code = get_huffman_code(value, table->u.node.left);

                        if (code == NULL)
                                code = get_huffman_code(value, table->u.node.right);

                /* A leaf has been reached */
                } else if (table->type == LEAF && value == table->u.val)
                        code = table;
        }

        return code;
}

/*
 * Writes a value using its Huffman code.
 */
bool write_huffman_value(int8_t value, struct huff_table *table,
                         struct bitstream *stream,
                         uint32_t **freqs, uint8_t freq_type)
{
        /* Compute value frequencies when required */
        if (freqs != NULL) {
                freqs[freq_type][(uint8_t)value]++;
                return true;
        }


        int8_t bit;
        bool success = false;

        struct huff_table *leaf = get_huffman_code(value, table);

        if (leaf != NULL) {
                uint32_t code = leaf->code;
                uint8_t size = leaf->size;

                /* Write the value's code, bit per bit */
                for (uint8_t i = 0; i < size; i++) {
                        bit = (code >> (size - 1 - i)) & 1;
                        write_bit(stream, bit, true);
                }

                success = true;
        }
        else
                printf("FATAL ERROR : no Huffman code for %d\n", value);


        return success;
}

/*
 * Computes a whole tree's codes and sizes
 */
static void compute_huffman_codes(struct huff_table *parent, bool *error)
{
        if (parent != NULL && parent->type != LEAF && !*error) {
                uint32_t code = parent->code << 1;
                uint8_t size = parent->size + 1;

                /*
                 * The package-merge Huffman construction
                 * algorithm could improve this
                 */
                if (size > 16) {
                        printf("Unable to create Huffman tree, try a higher compression rate\n");
                        *error = true;
                        return;
                }

                struct huff_table **left, **right;

                left = &parent->u.node.left;
                right = &parent->u.node.right;


                /* Compute left child's codes / sizes */
                if (*left != NULL) {
                        (*left)->code = code;
                        (*left)->size = size;

                        compute_huffman_codes(*left, error);
                }

                /* Compute right child's codes / sizes */
                if (*right != NULL) {
                        (*right)->code = code | 1;
                        (*right)->size = size;

                        compute_huffman_codes(*right, error);
                }
        }
}

/*
 * Deletes a specific node from the tree.
 */
static void delete_node(struct huff_table **table, struct huff_table *target)
{
        if (table != NULL && *table != NULL) {

                /* Once the target is found, delete it */
                if (*table == target) {
                        free_huffman_table(*table);
                        *table = NULL;

                /* Once the node is found, delete it */
                } else if ((*table)->type == NODE) {

                        delete_node(&(*table)->u.node.left, target);
                        delete_node(&(*table)->u.node.right, target);
                }
        }
}

/*
 * Creates a Huffman tree according
 * to given input frequencies.
 */
struct huff_table *create_huffman_tree(uint32_t freqs[0x100], bool *error)
{
        if (error != NULL && *error)
                return NULL;

        /* Create a priority queue */
        struct priority_queue *queue = create_queue(0x100);
        struct huff_table *node = NULL, *fake = NULL;

        if (queue == NULL)
                return NULL;

        /*
         * Null frequency value ensuring
         * no single code has only ones.
         * Indeed, libjpeg does not allow such a code.
         */
        fake = create_node(LEAF, 0, 0, 0);
        insert_queue(queue, 0, fake);

        /* Insert all values to the queue */
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


        /* Huffman tree construction */
        while (status) {
                status = best_queue(queue, &p1, &child0);

                /* If there's a minimal priority tree */
                if (status) {
                        delete_queue(queue);
                        status = best_queue(queue, &p2, &child1);
                }

                /* If there's a new minimal priority tree */
                if (status) {
                        delete_queue(queue);


                        node = create_node(NODE, 0, 0, 0);

                        if (node != NULL) {

                                node->u.node.left = child0;
                                node->u.node.right = child1;

                                /* Fuse both trees before adding them to the queue */
                                insert_queue(queue, p1 + p2, node);
                        } else
                                break;
                }

                /*
                * Else, the Huffman tree is
                * the last extracted one : child0
                */
                else
                        tree = child0;
        }

        /* Free the priority queue */
        free_queue(queue);

        /* Delete the fake node ensuring no code has only ones */
        delete_node(&tree, fake);


        /* Compute the final tree's codes and sizes */
        compute_huffman_codes(tree, error);


        return tree;
}

/*
 * Retrieves all Huffman values per size.
 */
static void forge_huffman_values(struct huff_table *table, uint8_t **values,
                          uint8_t *pos, uint8_t code_sizes[16])
{
        if (table != NULL) {

                /* Retrieve each leaf */
                if (table->type == LEAF){
                        uint8_t i = table->size - 1;

                        /* Overflow check */
                        if (pos[i] < code_sizes[i]) {
                                values[i][pos[i]] = table->u.val;
                                ++pos[i];       
                        }
                }

                /* Recursively retrieve each node's values */
                else if (table->type == NODE) {
                        forge_huffman_values(table->u.node.left, values, pos, code_sizes);
                        forge_huffman_values(table->u.node.right, values, pos, code_sizes);
                }
        }
}

/*
 * Counts how many codes of each size there are.
 */
static void count_code_sizes(struct huff_table *table, uint8_t *code_sizes)
{
        if (table != NULL) {

                /* Count each leaf */
                if (table->type == LEAF) {
                        uint8_t i = table->size - 1;

                        if (i < 16)
                                ++code_sizes[i];
                }

                /* Recursively count all nodes */
                else if (table->type == NODE) {
                        count_code_sizes(table->u.node.left, code_sizes);
                        count_code_sizes(table->u.node.right, code_sizes);
                }
        }
}

/*
 * Writes a Huffman table into the stream.
 */
void write_huffman_table(struct bitstream *stream, struct huff_table **itable)
{
        if (itable == NULL || *itable == NULL)
                return;

        uint8_t code_sizes[16];
        uint16_t nb_codes = 0;
        struct huff_table *table = *itable;

        /* Set all code sizes to 0 */
        memset(code_sizes, 0, sizeof(code_sizes));

        /* Count how many codes of each size there are. */
        count_code_sizes(table, code_sizes);

        /* Count how many codes there are */
        for (uint8_t i = 0; i < sizeof(code_sizes); ++i)
                nb_codes += code_sizes[i];


        /* There must be less than 256 different codes */
        if (nb_codes > 256) {
                printf("ERROR : more than 256 codes : %d\n", nb_codes);
                return;
        }


        /* Huffman values to write */
        uint8_t *values[16];

        /* Code size indexes (for computation) */
        uint8_t pos[16];

        /* Set all indexes to 0 */
        memset(pos, 0, sizeof(pos));

        /* Allocate enough space for each Huffman value */
        for (uint8_t i = 0; i < sizeof(code_sizes); ++i)
                values[i] = calloc(1, code_sizes[i]);


        /* Retrieve all Huffman values to write them in the correct order */
        forge_huffman_values(table, values, pos, code_sizes);

        /*
         * Free the previous Huffman tree
         * (the previous generated one's codes are incorrectly ordered)
         */
        free_huffman_table(table);


        /* Rewrite the Huffman tree correctly,
         * with smallest codes on the left */
        table = calloc(1, sizeof(struct huff_table));
        *itable = table;


        /* Write the number of Huffman codes per size */
        for (uint8_t i = 0; i < sizeof(code_sizes); i++)
                write_byte(stream, code_sizes[i]);


        /* Write all Huffman values in the right order */
        for (uint8_t i = 0; i < sizeof(code_sizes); ++i) {
                for (uint8_t j = 0; j < code_sizes[i]; ++j) {
                        write_byte(stream, values[i][j]);
                        add_huffman_code(values[i][j], i, table);
                }
        }

        /* Free allocated memory */
        for (uint8_t i = 0; i < sizeof(code_sizes); ++i)
                SAFE_FREE(values[i]);
}


/*
 * Recursive dot export procedure.
 */
void huffman_export_rec(FILE *file, struct huff_table *table, uint32_t *index)
{
        if (table != NULL && index) {
                uint32_t cur = *index + 1;

                *index = cur;

                fprintf(file, " -- %u", cur);

                if (table->type == LEAF) {
                        fprintf(file, "    %u [label=\"%u : %02X => %2X\"]\n", cur,
                                table->size, table->code, (uint8_t)table->u.val);
                } else {
                        huffman_export_rec(file, table->u.node.left, index);
                        fprintf(file, "    %u [label=\"%02X\"]\n", cur, table->code);
                        fprintf(file, "    %u", cur);
                        huffman_export_rec(file, table->u.node.right, index);
                }
        } else {
                fprintf(file, "\n");
        }
}

/*
 * Exports the input Huffman table to
 * a dot tree file (for debugging).
 */
void huffman_export(char *dest, struct huff_table *table)
{
        FILE *file = NULL;
        uint32_t index = 0;
        uint32_t cur = index;

        file = fopen(dest, "w");

        if (table != NULL && file != NULL) {
                fprintf(file, "\ngraph {\n");

                if (table->type == LEAF) {
                        fprintf(file, "    %u [label=\"%i\"]\n",
                                cur, table->u.val);
                } else {
                        fprintf(file, "    %u \n", cur);
                        fprintf(file, "    %u [label=\"ε\"]\n", cur);
                }

                fprintf(file, "    %u", cur);
                huffman_export_rec(file, table->u.node.left, &index);
                fprintf(file, "    %u", cur);
                huffman_export_rec(file, table->u.node.right, &index);
                fprintf(file, "}\n\n");

                fclose(file);
        }
}


