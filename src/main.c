#include "common.h"
#include "logger.h"
#include "metrics.h"
#include "parser.h"
#include "queue.h"
#include "scheduler.h"
#include "sync.h"
#include "worker.h"

#include <stdio.h>
#include <stdlib.h>

static void context_request_shutdown(scheduler_context_t *ctx) {
    pthread_mutex_lock(&ctx->mutex);
    ctx->shutdown = 1;
    pthread_cond_broadcast(&ctx->job_available);
    pthread_mutex_unlock(&ctx->mutex);
}

static int push_arrived_jobs(scheduler_context_t *ctx, int *index) {
    int arrival_time = ctx->jobs[*index].arrival_time;

    pthread_mutex_lock(&ctx->mutex);
    while (*index < ctx->job_count && ctx->jobs[*index].arrival_time == arrival_time) {
        job_t *job = &ctx->jobs[*index];

        job->status = JOB_WAITING;
        job->sequence_no = ctx->sequence_counter++;
        if (queue_push(&ctx->ready_queue, job) != 0) {
            pthread_mutex_unlock(&ctx->mutex);
            return -1;
        }

        (*index)++;
    }

    pthread_cond_broadcast(&ctx->job_available);
    pthread_mutex_unlock(&ctx->mutex);
    return 0;
}

static int run_arrival_loop(scheduler_context_t *ctx) {
    int index = 0;

    while (index < ctx->job_count) {
        int arrival_time = ctx->jobs[index].arrival_time;

        sleep_until_sim_time(ctx->start_wall_ms, arrival_time);
        if (push_arrived_jobs(ctx, &index) != 0) {
            return -1;
        }
    }

    return 0;
}

static void wait_until_all_done_and_shutdown(scheduler_context_t *ctx) {
    pthread_mutex_lock(&ctx->mutex);
    while (ctx->completed_jobs < ctx->job_count) {
        pthread_cond_wait(&ctx->all_done, &ctx->mutex);
    }

    ctx->shutdown = 1;
    pthread_cond_broadcast(&ctx->job_available);
    pthread_mutex_unlock(&ctx->mutex);
}

static int context_init(scheduler_context_t *ctx,
                        job_t *jobs,
                        int job_count,
                        int worker_count,
                        policy_t policy) {
    *ctx = (scheduler_context_t){0};
    ctx->jobs = jobs;
    ctx->job_count = job_count;
    ctx->worker_count = worker_count;
    ctx->policy = policy;
    ctx->start_wall_ms = monotonic_ms();

    if (queue_init(&ctx->ready_queue, job_count) != 0) {
        return -1;
    }

    if (sync_init(ctx) != 0) {
        queue_destroy(&ctx->ready_queue);
        return -1;
    }

    return 0;
}

static void context_destroy(scheduler_context_t *ctx) {
    sync_destroy(ctx);
    queue_destroy(&ctx->ready_queue);
}

int main(int argc, char **argv) {
    const char *csv_path;
    policy_t policy;
    int worker_count;
    job_t *jobs = NULL;
    int job_count = 0;
    worker_t *workers = NULL;
    scheduler_context_t ctx;
    metrics_t metrics;
    int exit_code = 1;

    if (argc != 4) {
        print_usage(argv[0]);
        return 1;
    }

    csv_path = argv[1];
    if (parse_policy(argv[2], &policy) != 0) {
        fprintf(stderr, "Invalid policy: %s\n", argv[2]);
        print_usage(argv[0]);
        return 1;
    }

    if (parse_positive_int(argv[3], &worker_count) != 0) {
        fprintf(stderr, "Invalid worker count: %s\n", argv[3]);
        print_usage(argv[0]);
        return 1;
    }

    if (load_jobs_from_csv(csv_path, &jobs, &job_count) != 0) {
        return 1;
    }

    sort_jobs_by_arrival_then_id(jobs, job_count);

    workers = calloc((size_t)worker_count, sizeof(worker_t));
    if (workers == NULL) {
        fprintf(stderr, "Failed to allocate workers\n");
        free_jobs(jobs);
        return 1;
    }

    if (context_init(&ctx, jobs, job_count, worker_count, policy) != 0) {
        fprintf(stderr, "Failed to initialize scheduler context\n");
        free(workers);
        free_jobs(jobs);
        return 1;
    }

    if (workers_start(workers, worker_count, &ctx) != 0) {
        fprintf(stderr, "Failed to start worker threads\n");
        context_destroy(&ctx);
        free(workers);
        free_jobs(jobs);
        return 1;
    }

    if (run_arrival_loop(&ctx) != 0) {
        fprintf(stderr, "Failed to enqueue arrived jobs\n");
        context_request_shutdown(&ctx);
        workers_join(workers, worker_count);
        context_destroy(&ctx);
        free(workers);
        free_jobs(jobs);
        return 1;
    }

    wait_until_all_done_and_shutdown(&ctx);

    if (workers_join(workers, worker_count) != 0) {
        fprintf(stderr, "Failed to join all worker threads\n");
        goto cleanup;
    }

    metrics = metrics_calculate(&ctx);
    metrics_print_summary(&ctx, &metrics);
    exit_code = 0;

cleanup:
    context_destroy(&ctx);
    free(workers);
    free_jobs(jobs);
    return exit_code;
}
