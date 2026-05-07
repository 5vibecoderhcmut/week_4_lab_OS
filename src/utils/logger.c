#include "logger.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_worker_start(int sim_time, int worker_id, const job_t *job) {
    if (job == NULL) {
        return;
    }

    pthread_mutex_lock(&log_mutex);
    printf("[time=%d] Worker %d starts Job %d seller=%s runtime=%d priority=%d type=%s\n",
           sim_time,
           worker_id,
           job->job_id,
           job->seller_id,
           job->estimated_runtime,
           job->priority,
           job->job_type);
    fflush(stdout);
    pthread_mutex_unlock(&log_mutex);
}

void log_worker_finish(int sim_time, int worker_id, const job_t *job) {
    if (job == NULL) {
        return;
    }

    pthread_mutex_lock(&log_mutex);
    printf("[time=%d] Worker %d finishes Job %d\n", sim_time, worker_id, job->job_id);
    fflush(stdout);
    pthread_mutex_unlock(&log_mutex);
}

void log_error(const char *fmt, ...) {
    va_list args;

    pthread_mutex_lock(&log_mutex);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fflush(stderr);
    pthread_mutex_unlock(&log_mutex);
}
