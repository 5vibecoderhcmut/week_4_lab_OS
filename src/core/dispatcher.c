#include "dispatcher.h"

void dispatcher_assign_job(worker_t *worker, job_t *job, int start_time) {
    if (worker == NULL || job == NULL) {
        return;
    }

    job->status = JOB_RUNNING;
    job->start_time = start_time;
    job->assigned_worker_id = worker->worker_id;
}
