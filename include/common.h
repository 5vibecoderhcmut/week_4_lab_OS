#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>

#include "queue.h"

typedef enum {
    POLICY_FIFO = 0,
    POLICY_SJF,
    POLICY_PRIORITY,
    POLICY_AGING
} policy_t;

typedef struct {
    job_t *jobs;
    int job_count;
    int completed_jobs;
    int worker_count;
    policy_t policy;

    job_queue_t ready_queue;
    pthread_mutex_t mutex;
    pthread_cond_t job_available;
    pthread_cond_t all_done;

    int shutdown;
    long start_wall_ms;
    long total_worker_busy_time;
    long sequence_counter;
} scheduler_context_t;

int parse_positive_int(const char *text, int *out);
void sort_jobs_by_arrival_then_id(job_t *jobs, int count);
void print_usage(const char *program);

long monotonic_ms(void);
long elapsed_sim_time(long start_wall_ms);
void sleep_ms(long ms);
void sleep_sim_units(int units);
void sleep_until_sim_time(long start_wall_ms, int target_time);

#endif
