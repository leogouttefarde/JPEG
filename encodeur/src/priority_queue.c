
#include "priority_queue.h"


/*
 * Queue element
 */
struct element {

        /* Priority */
        uint32_t priority;

        /* Huffman data */
        struct huff_table *table;
};

/*
 * Priority queue
 */
struct priority_queue {

        /* Queue's heap */
        struct element *heap;

        /* Heap's max size */
        uint32_t max_size;

        /* Number of elements */
        uint32_t size;
};


/*
 * Creates and returns a new queue
 * able to contain maximum max_size elements.
 */
struct priority_queue *create_queue(uint32_t max_size)
{
        struct priority_queue *queue = NULL;

        queue = calloc(1, sizeof(struct priority_queue));

        if (queue != NULL) {
                queue->heap = calloc(max_size, sizeof(struct element));
                queue->max_size = max_size;
                queue->size = 0;

                if (queue->heap == NULL)
                        SAFE_FREE(queue);
        }

        return queue;
}

/*
 * Inserts an element with the given priority.
 */
bool insert_queue(struct priority_queue *queue, uint32_t priority, struct huff_table *table)
{
        struct element elem;
        uint32_t i_cur, i_parent;
        struct element *heap = NULL;

        if (queue != NULL && queue->size < queue->max_size) {

                heap = queue->heap;

                /* We add the element at the next position */
                i_cur = ++queue->size;
                heap[i_cur - 1] = (struct element){ priority, table };

                /*
                 * Place the new element in the tree :
                 * as long as it's better as its father, switch them
                 */
                 i_parent = i_cur / 2;
                 while (i_cur > 1 && (heap[i_cur-1].priority < heap[i_parent - 1].priority)) {
                        elem = heap[i_cur - 1];

                        heap[i_cur - 1] = heap[i_parent - 1];
                        heap[i_parent - 1] = elem;

                        i_cur = i_parent;
                        i_parent = i_cur / 2;
                 }

                 return true;
        }

        return false;
}

/*
 * Retrieves the best priority element (leaves it in the queue).
 * Returns false if the queue is empty.
 */
bool best_queue(struct priority_queue *queue, uint32_t *priority, struct huff_table **table)
{
        /*
         * If there's an element in the queue,
         * return the first one (and so the best)
         */
        if (queue != NULL && queue->size > 0 && priority && table) {
                *priority = queue->heap[0].priority;
                *table = queue->heap[0].table;

                return true;
        }

        return false;
}

/*
 * Deletes the best priority element
 */
bool delete_queue(struct priority_queue *queue)
{
        bool fin = false;
        struct element elem;
        uint32_t i_cur, child;
        struct element *heap = NULL;

        /*
         * If there's an element in the queue,
         * delete the first one and reorganize the queue
         */
        if (queue != NULL && queue->size > 0) {

                heap = queue->heap;

                /* Place the last element in the beginning */
                heap[0] = heap[--queue->size];

                /* Reorganize the queue until a leaf is reached */
                i_cur = 1;
                while ((i_cur <= (queue->size / 2)) && !fin) {

                        child = 2 * i_cur;

                        /*
                         * If the left child isn't the last element
                         * and isn't as good as the right child,
                         * we will compare the father to the right child
                         */
                        if ((child != queue->size) && (heap[child-1].priority >= heap[child].priority))
                                child++;

                        /*
                         * If the child to compare is better
                         * than its father, switch them
                         */
                        if (heap[i_cur-1].priority > heap[child-1].priority) {
                                elem = heap[i_cur-1];
                                heap[i_cur-1] = heap[child-1];
                                heap[child-1] = elem;
                                i_cur = child;

                        /* Else, reorganization finished */
                        } else
                                fin = true;
                }


                return true;
        }

        return false;
}

/*
 * Frees a queue
 */
void free_queue(struct priority_queue *queue)
{
        if (queue != NULL)
                SAFE_FREE(queue->heap);

        SAFE_FREE(queue);
}

