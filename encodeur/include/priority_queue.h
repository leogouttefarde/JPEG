/* Projet C - Sujet JPEG */
#ifndef __PRIORITY_QUEUE_H__
#define __PRIORITY_QUEUE_H__

#include "huffman.h"
#include "common.h"


/*
 * Priority queue
 */
struct priority_queue;


/*
 * Creates and returns a new queue
 * able to contain maximum max_size elements.
 */
struct priority_queue *create_queue(uint32_t max_size);

/*
 * Inserts an element with the given priority.
 */
bool insert_queue(struct priority_queue *queue, uint32_t priority, struct huff_table *table);

/*
 * Retrieves the best priority element (leaves it in the queue).
 * Returns false if the queue is empty.
 */
bool best_queue(struct priority_queue *queue, uint32_t *priority, struct huff_table **table);

/*
 * Deletes the best priority element
 */
bool delete_queue(struct priority_queue *queue);

/*
 * Frees a queue
 */
void free_queue(struct priority_queue *queue);


#endif
