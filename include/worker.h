#ifndef WORKER_H
#define WORKER_H

#include <pthread.h>

typedef struct {
    int worker_id;
    pthread_t thread;
    void *ctx;
    long busy_time;
} worker_t;

int workers_start(worker_t *workers, int worker_count, void *ctx);
int workers_join(worker_t *workers, int worker_count);
void *worker_loop(void *arg);

#endif
