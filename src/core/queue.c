#include "queue.h"

#include <stdlib.h>

int queue_init(job_queue_t *queue, int capacity) {
    if (queue == NULL) {
        return -1;
    }

    if (capacity < 1) {
        capacity = 1;
    }

    queue->items = calloc((size_t)capacity, sizeof(job_t *));
    if (queue->items == NULL) {
        queue->size = 0;
        queue->capacity = 0;
        return -1;
    }

    queue->size = 0;
    queue->capacity = capacity;
    return 0;
}

void queue_destroy(job_queue_t *queue) {
    if (queue == NULL) {
        return;
    }

    free(queue->items);
    queue->items = NULL;
    queue->size = 0;
    queue->capacity = 0;
}

int queue_push(job_queue_t *queue, job_t *job) {
    if (queue == NULL || job == NULL) {
        return -1;
    }

    if (queue->size >= queue->capacity) {
        int new_capacity = queue->capacity > 0 ? queue->capacity * 2 : 1;
        job_t **new_items = realloc(queue->items, (size_t)new_capacity * sizeof(job_t *));
        if (new_items == NULL) {
            return -1;
        }

        queue->items = new_items;
        queue->capacity = new_capacity;
    }

    queue->items[queue->size] = job;
    queue->size++;
    return 0;
}

job_t *queue_remove_at(job_queue_t *queue, int index) {
    if (queue == NULL || index < 0 || index >= queue->size) {
        return NULL;
    }

    job_t *job = queue->items[index];
    for (int i = index; i < queue->size - 1; i++) {
        queue->items[i] = queue->items[i + 1];
    }

    queue->size--;
    queue->items[queue->size] = NULL;
    return job;
}

job_t *queue_peek_at(const job_queue_t *queue, int index) {
    if (queue == NULL || index < 0 || index >= queue->size) {
        return NULL;
    }

    return queue->items[index];
}

int queue_is_empty(const job_queue_t *queue) {
    return queue == NULL || queue->size == 0;
}
