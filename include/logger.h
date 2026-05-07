#ifndef LOGGER_H
#define LOGGER_H

#include "job.h"

void log_worker_start(int sim_time, int worker_id, const job_t *job);
void log_worker_finish(int sim_time, int worker_id, const job_t *job);
void log_error(const char *fmt, ...);

#endif
