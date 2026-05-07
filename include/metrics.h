#ifndef METRICS_H
#define METRICS_H

#include "common.h"

typedef struct {
    int total_jobs;
    int total_simulation_time;
    double average_waiting_time;
    double average_turnaround_time;
    double throughput;
    double worker_utilization_percent;
    int starvation_risk_jobs;
    double starvation_threshold;
} metrics_t;

metrics_t metrics_calculate(const scheduler_context_t *ctx);
void metrics_print_summary(const scheduler_context_t *ctx, const metrics_t *metrics);

#endif
