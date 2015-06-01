
#include "priority_queue.h"


struct element {
        uint32_t priority;
        struct huff_table *table;
};

struct priority_queue {
        struct element *heap;
        uint32_t max_size;
        uint32_t size;
};


/*
 * Cree et retourne une nouvelle file
 * pouvant contenir au plus 'Taille' elements
 */
struct priority_queue *create_queue(uint32_t max_size)
{
        struct priority_queue *queue = NULL;

        queue = calloc(1, sizeof(struct priority_queue));

        if (queue != NULL) {
                queue->heap = calloc(max_size+1, sizeof(struct element));
                queue->max_size = max_size;
                queue->size = 0;

                if (queue->heap == NULL)
                        SAFE_FREE(queue);
        }

        return queue;
}

/*
 * Insère un élément de priorité donnée
 */
bool insert_queue(struct priority_queue *queue, uint32_t priority, struct huff_table *table)
{
        struct element elem;
        uint32_t i_cur, i_parent;
        struct element *heap = NULL;

        if (queue != NULL && queue->size < queue->max_size) {

                heap = queue->heap;

                /* On ajoute l'élément à la prochaine position */
                i_cur = ++queue->size;
                heap[i_cur] = (struct element){ priority, table };

                /*
                 * On place le nouvel élément dans l'arbre :
                 * tant qu'il est meilleur que son père, on les permute
                 */
                 i_parent = i_cur / 2;
                 while (i_cur > 1 && (heap[i_cur].priority < heap[i_parent].priority)) {
                        elem = heap[i_cur];

                        heap[i_cur] = heap[i_parent];
                        heap[i_parent] = elem;

                        i_cur = i_parent;
                        i_parent = i_cur / 2;
                 }

                 return true;
        }

        return false;
}

/*
 * Récupère l'élément de meilleure priorité (le laisse dans la file)
 * Met le statut à Faux si la file est vide
 */
bool best_queue(struct priority_queue *queue, uint32_t *priority, struct huff_table **table)
{
        if (queue != NULL && queue->size > 0 && priority && table) {
                *priority = queue->heap[1].priority;
                *table = queue->heap[1].table;

                return true;
        }

        return false;
}

/*
 * Supprime l'élément de meilleure priorité
 */
bool delete_queue(struct priority_queue *queue)
{
        bool fin = false;
        struct element elem;
        uint32_t i_cur, child;
        struct element *heap = NULL;

        if (queue != NULL && queue->size > 0) {

                heap = queue->heap;

                heap[1] = heap[queue->size--];

                i_cur = 1;

                while ((i_cur <= (queue->size / 2)) && !fin) {

                        child = 2 * i_cur;

                        if ((child != queue->size) && (heap[child].priority >= heap[child + 1].priority))
                                child++;

                        if (heap[i_cur].priority > heap[child].priority) {
                                elem = heap[i_cur];
                                heap[i_cur] = heap[child];
                                heap[child] = elem;
                                i_cur = child;
                        } else
                                fin = true;
                }


                return true;
        }

        return false;
}

void free_queue(struct priority_queue *queue)
{
        if (queue != NULL)
                SAFE_FREE(queue->heap);

        SAFE_FREE(queue);
}

