#include "metrics.h"
#include "scheduler.h"

#include <stdio.h>

metrics_t metrics_calculate(const scheduler_context_t *ctx) {
    metrics_t metrics = {0};
    double total_waiting_time = 0.0;
    double total_turnaround_time = 0.0;
    double total_runtime = 0.0;
    int max_finish_time = 0;

    if (ctx == NULL || ctx->jobs == NULL || ctx->job_count <= 0) {
        return metrics;
    }

    metrics.total_jobs = ctx->job_count;

    for (int i = 0; i < ctx->job_count; i++) {
        const job_t *job = &ctx->jobs[i];
        int waiting_time = job->start_time - job->arrival_time;
        int turnaround_time = job->finish_time - job->arrival_time;

        if (waiting_time < 0) {
            waiting_time = 0;
        }

        if (turnaround_time < 0) {
            turnaround_time = 0;
        }

        total_waiting_time += waiting_time;
        total_turnaround_time += turnaround_time;
        total_runtime += job->estimated_runtime;

        if (job->finish_time > max_finish_time) {
            max_finish_time = job->finish_time;
        }
    }

    metrics.total_simulation_time = max_finish_time > 0 ? max_finish_time : 1;
    metrics.average_waiting_time = total_waiting_time / ctx->job_count;
    metrics.average_turnaround_time = total_turnaround_time / ctx->job_count;
    metrics.throughput = (double)ctx->job_count / metrics.total_simulation_time;

    if (ctx->worker_count > 0 && metrics.total_simulation_time > 0) {
        metrics.worker_utilization_percent =
            ((double)ctx->total_worker_busy_time /
             (ctx->worker_count * metrics.total_simulation_time)) *
            100.0;
    }

    metrics.starvation_threshold = STARVATION_MULTIPLIER * (total_runtime / ctx->job_count);
    for (int i = 0; i < ctx->job_count; i++) {
        int waiting_time = ctx->jobs[i].start_time - ctx->jobs[i].arrival_time;
        if (waiting_time > metrics.starvation_threshold) {
            metrics.starvation_risk_jobs++;
        }
    }

    return metrics;
}

void metrics_print_summary(const scheduler_context_t *ctx, const metrics_t *metrics) {
    if (ctx == NULL || metrics == NULL) {
        return;
    }

    printf("\n");
    printf("Policy: %s\n", policy_to_string(ctx->policy));
    printf("Workers: %d\n", ctx->worker_count);
    printf("Total jobs: %d\n", metrics->total_jobs);
    printf("Total simulation time: %d\n", metrics->total_simulation_time);
    printf("\n");
    printf("Average waiting time: %.2f\n", metrics->average_waiting_time);
    printf("Average turnaround time: %.2f\n", metrics->average_turnaround_time);
    printf("Throughput: %.3f jobs/unit time\n", metrics->throughput);
    printf("Worker utilization: %.2f%%\n", metrics->worker_utilization_percent);
    printf("Starvation threshold: %.2f\n", metrics->starvation_threshold);
    printf("Starvation-risk jobs: %d\n", metrics->starvation_risk_jobs);
}
