
#include "priority_queue.h"


int main()
{
        struct priority_queue *queue = create_queue(10);

        if (queue == NULL)
                printf("create_queue error\n");

        insert_queue(queue, 10, (struct huff_table*)10);
        insert_queue(queue, 11, (struct huff_table*)11);
        insert_queue(queue, 8, (struct huff_table*)8);
        insert_queue(queue, 3, (struct huff_table*)3);
        insert_queue(queue, 7, (struct huff_table*)7);
        insert_queue(queue, 15, (struct huff_table*)15);

        uint32_t priority = 0;
        struct huff_table *table = NULL;

        while (best_queue(queue, &priority, &table)) {

                printf("Table = %ld\n", (uint64_t)table);
                delete_queue(queue);
        }

        free_queue(queue);


        return EXIT_SUCCESS;
}


