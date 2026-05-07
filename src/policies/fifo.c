#include "queue.h"

#include <stddef.h>

int policy_select_fifo(const job_queue_t *queue) {
    int best = 0;

    if (queue == NULL || queue->size <= 0) {
        return -1;
    }

    for (int i = 1; i < queue->size; i++) {
        if (queue->items[i]->sequence_no < queue->items[best]->sequence_no) {
            best = i;
        }
    }

    return best;
}
