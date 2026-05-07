#ifndef QUEUE_H
#define QUEUE_H

#include "job.h"

typedef struct {
    job_t **items;
    int size;
    int capacity;
} job_queue_t;

int queue_init(job_queue_t *queue, int capacity);
void queue_destroy(job_queue_t *queue);
int queue_push(job_queue_t *queue, job_t *job);
job_t *queue_remove_at(job_queue_t *queue, int index);
job_t *queue_peek_at(const job_queue_t *queue, int index);
int queue_is_empty(const job_queue_t *queue);

#endif
