#ifndef JOB_H
#define JOB_H

#include "config.h"

typedef enum {
    JOB_NEW = 0,
    JOB_WAITING,
    JOB_RUNNING,
    JOB_DONE
} job_status_t;

typedef struct {
    int job_id;
    char seller_id[MAX_SELLER_ID_LEN];
    int arrival_time;
    int estimated_runtime;
    int priority;
    char job_type[MAX_JOB_TYPE_LEN];

    int start_time;
    int finish_time;
    job_status_t status;
    int assigned_worker_id;
    long sequence_no;
} job_t;

#endif
