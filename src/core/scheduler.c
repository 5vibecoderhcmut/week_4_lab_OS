#include "scheduler.h"

#include <strings.h>

int policy_select_fifo(const job_queue_t *queue);
int policy_select_sjf(const job_queue_t *queue);
int policy_select_priority(const job_queue_t *queue);
int policy_select_aging(const job_queue_t *queue, int now);

int parse_policy(const char *text, policy_t *out_policy) {
    if (text == NULL || out_policy == NULL) {
        return -1;
    }

    if (strcasecmp(text, "fifo") == 0 || strcasecmp(text, "fcfs") == 0) {
        *out_policy = POLICY_FIFO;
        return 0;
    }

    if (strcasecmp(text, "sjf") == 0 || strcasecmp(text, "sif") == 0) {
        *out_policy = POLICY_SJF;
        return 0;
    }

    if (strcasecmp(text, "priority") == 0 || strcasecmp(text, "prio") == 0) {
        *out_policy = POLICY_PRIORITY;
        return 0;
    }

    if (strcasecmp(text, "aging") == 0 || strcasecmp(text, "aging-priority") == 0) {
        *out_policy = POLICY_AGING;
        return 0;
    }

    return -1;
}

const char *policy_to_string(policy_t policy) {
    switch (policy) {
        case POLICY_FIFO:
            return "FIFO";
        case POLICY_SJF:
            return "SJF";
        case POLICY_PRIORITY:
            return "Priority";
        case POLICY_AGING:
            return "Aging Priority";
        default:
            return "Unknown";
    }
}

job_t *scheduler_get_next_job(scheduler_context_t *ctx) {
    int selected_index = 0;

    if (ctx == NULL || queue_is_empty(&ctx->ready_queue)) {
        return NULL;
    }

    switch (ctx->policy) {
        case POLICY_FIFO:
            selected_index = policy_select_fifo(&ctx->ready_queue);
            break;
        case POLICY_SJF:
            selected_index = policy_select_sjf(&ctx->ready_queue);
            break;
        case POLICY_PRIORITY:
            selected_index = policy_select_priority(&ctx->ready_queue);
            break;
        case POLICY_AGING:
            selected_index = policy_select_aging(&ctx->ready_queue,
                                                 (int)elapsed_sim_time(ctx->start_wall_ms));
            break;
        default:
            selected_index = 0;
            break;
    }

    return queue_remove_at(&ctx->ready_queue, selected_index);
}
