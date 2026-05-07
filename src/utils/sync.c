#include "sync.h"

int sync_init(scheduler_context_t *ctx) {
    if (ctx == NULL) {
        return -1;
    }

    if (pthread_mutex_init(&ctx->mutex, NULL) != 0) {
        return -1;
    }

    if (pthread_cond_init(&ctx->job_available, NULL) != 0) {
        pthread_mutex_destroy(&ctx->mutex);
        return -1;
    }

    if (pthread_cond_init(&ctx->all_done, NULL) != 0) {
        pthread_cond_destroy(&ctx->job_available);
        pthread_mutex_destroy(&ctx->mutex);
        return -1;
    }

    return 0;
}

void sync_destroy(scheduler_context_t *ctx) {
    if (ctx == NULL) {
        return;
    }

    pthread_cond_destroy(&ctx->all_done);
    pthread_cond_destroy(&ctx->job_available);
    pthread_mutex_destroy(&ctx->mutex);
}
