#include "queue.h"

#include <stddef.h>

static int is_better_priority_job(const job_t *candidate, const job_t *best) {
    if (candidate->priority != best->priority) {
        return candidate->priority < best->priority;
    }

    if (candidate->arrival_time != best->arrival_time) {
        return candidate->arrival_time < best->arrival_time;
    }

    if (candidate->sequence_no != best->sequence_no) {
        return candidate->sequence_no < best->sequence_no;
    }

    return candidate->job_id < best->job_id;
}

int policy_select_priority(const job_queue_t *queue) {
    int best = 0;

    if (queue == NULL || queue->size <= 0) {
        return -1;
    }

    for (int i = 1; i < queue->size; i++) {
        if (is_better_priority_job(queue->items[i], queue->items[best])) {
            best = i;
        }
    }

    return best;
}
