#include "worker.h"

#include "common.h"
#include "dispatcher.h"
#include "logger.h"
#include "queue.h"
#include "scheduler.h"

int workers_start(worker_t *workers, int worker_count, void *ctx) {
    scheduler_context_t *scheduler_ctx = ctx;

    if (workers == NULL || worker_count <= 0 || ctx == NULL) {
        return -1;
    }

    for (int i = 0; i < worker_count; i++) {
        workers[i].worker_id = i;
        workers[i].ctx = ctx;
        workers[i].busy_time = 0;

        if (pthread_create(&workers[i].thread, NULL, worker_loop, &workers[i]) != 0) {
            pthread_mutex_lock(&scheduler_ctx->mutex);
            scheduler_ctx->shutdown = 1;
            pthread_cond_broadcast(&scheduler_ctx->job_available);
            pthread_mutex_unlock(&scheduler_ctx->mutex);

            for (int j = 0; j < i; j++) {
                pthread_join(workers[j].thread, NULL);
            }
            return -1;
        }
    }

    return 0;
}

int workers_join(worker_t *workers, int worker_count) {
    int status = 0;

    if (workers == NULL || worker_count <= 0) {
        return -1;
    }

    for (int i = 0; i < worker_count; i++) {
        if (pthread_join(workers[i].thread, NULL) != 0) {
            status = -1;
        }
    }

    return status;
}

void *worker_loop(void *arg) {
    worker_t *worker = arg;
    scheduler_context_t *ctx;

    if (worker == NULL || worker->ctx == NULL) {
        return NULL;
    }

    ctx = worker->ctx;

    while (1) {
        job_t *job;
        int start_time;
        int finish_time;

        pthread_mutex_lock(&ctx->mutex);

        while (queue_is_empty(&ctx->ready_queue) && !ctx->shutdown) {
            pthread_cond_wait(&ctx->job_available, &ctx->mutex);
        }

        if (ctx->shutdown && queue_is_empty(&ctx->ready_queue)) {
            pthread_mutex_unlock(&ctx->mutex);
            break;
        }

        job = scheduler_get_next_job(ctx);
        if (job == NULL) {
            pthread_mutex_unlock(&ctx->mutex);
            continue;
        }

        start_time = (int)elapsed_sim_time(ctx->start_wall_ms);
        if (start_time < job->arrival_time) {
            start_time = job->arrival_time;
        }
        dispatcher_assign_job(worker, job, start_time);

        pthread_mutex_unlock(&ctx->mutex);

        log_worker_start(start_time, worker->worker_id, job);
        sleep_sim_units(job->estimated_runtime);

        finish_time = job->start_time + job->estimated_runtime;
        log_worker_finish(finish_time, worker->worker_id, job);

        pthread_mutex_lock(&ctx->mutex);
        job->finish_time = finish_time;
        job->status = JOB_DONE;
        worker->busy_time += job->estimated_runtime;
        ctx->total_worker_busy_time += job->estimated_runtime;
        ctx->completed_jobs++;
        if (ctx->completed_jobs == ctx->job_count) {
            pthread_cond_signal(&ctx->all_done);
        }
        pthread_mutex_unlock(&ctx->mutex);
    }

    return NULL;
}
