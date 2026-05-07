#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static int compare_jobs_by_arrival_then_id(const void *left, const void *right) {
    const job_t *a = left;
    const job_t *b = right;

    if (a->arrival_time != b->arrival_time) {
        return a->arrival_time - b->arrival_time;
    }

    return a->job_id - b->job_id;
}

int parse_positive_int(const char *text, int *out) {
    char *end = NULL;
    long value;

    if (text == NULL || *text == '\0' || out == NULL) {
        return -1;
    }

    errno = 0;
    value = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value <= 0 || value > 1000000) {
        return -1;
    }

    *out = (int)value;
    return 0;
}

void sort_jobs_by_arrival_then_id(job_t *jobs, int count) {
    if (jobs == NULL || count <= 1) {
        return;
    }

    qsort(jobs, (size_t)count, sizeof(job_t), compare_jobs_by_arrival_then_id);
}

void print_usage(const char *program) {
    fprintf(stderr, "Usage: %s <jobs.csv> <policy> <workers>\n", program);
    fprintf(stderr, "Policies: fifo, fcfs, sjf, priority, aging\n");
    fprintf(stderr, "Example: %s workloads/workload_a.csv fifo 4\n", program);
}
