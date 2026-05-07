#include "config.h"
#include "queue.h"

#include <stddef.h>

static int effective_priority(const job_t *job, int now) {
    int waiting_time = now - job->arrival_time;
    int boost;
    int priority;

    if (waiting_time < 0) {
        waiting_time = 0;
    }

    boost = waiting_time / AGING_INTERVAL;
    priority = job->priority - boost;
    return priority < 0 ? 0 : priority;
}

static int is_better_aging_job(const job_t *candidate, const job_t *best, int now) {
    int candidate_priority = effective_priority(candidate, now);
    int best_priority = effective_priority(best, now);
    int candidate_wait = now - candidate->arrival_time;
    int best_wait = now - best->arrival_time;

    if (candidate_priority != best_priority) {
        return candidate_priority < best_priority;
    }

    if (candidate_wait != best_wait) {
        return candidate_wait > best_wait;
    }

    if (candidate->sequence_no != best->sequence_no) {
        return candidate->sequence_no < best->sequence_no;
    }

    return candidate->job_id < best->job_id;
}

int policy_select_aging(const job_queue_t *queue, int now) {
    int best = 0;

    if (queue == NULL || queue->size <= 0) {
        return -1;
    }

    for (int i = 1; i < queue->size; i++) {
        if (is_better_aging_job(queue->items[i], queue->items[best], now)) {
            best = i;
        }
    }

    return best;
}
