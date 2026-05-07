#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "job.h"
#include "worker.h"

void dispatcher_assign_job(worker_t *worker, job_t *job, int start_time);

#endif
