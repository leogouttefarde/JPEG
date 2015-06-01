/* Projet C - Sujet JPEG */
#ifndef __PRIORITY_QUEUE_H__
#define __PRIORITY_QUEUE_H__

#include "huffman.h"
#include "common.h"


struct priority_queue;


/*
 * Cree et retourne une nouvelle file
 * pouvant contenir au plus 'max_size' elements
 */
struct priority_queue *create_queue(uint32_t max_size);

/*
 * Insère un élément de priorité donnée
 */
bool insert_queue(struct priority_queue *queue, uint32_t priority, struct huff_table *table);

/*
 * Récupère l'élément de meilleure priorité (le laisse dans la file)
 * Met le statut à Faux si la file est vide
 */
bool best_queue(struct priority_queue *queue, uint32_t *priority, struct huff_table **table);

/*
 * Supprime l'élément de meilleure priorité
 */
bool delete_queue(struct priority_queue *queue);

/*
 * Libère une file
 */
void free_queue(struct priority_queue *queue);


#endif
