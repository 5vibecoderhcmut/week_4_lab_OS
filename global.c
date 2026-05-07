#include "scheduler.h"

/*
    ============================================================
    GLOBAL VARIABLE DEFINITIONS
    ============================================================

    Những biến này được dùng chung bởi nhiều file:
    - input_reader.c
    - worker.c
    - scheduler_policy.c
    - metrics.c
    - main.c

    Trong scheduler.h chỉ khai báo extern.
    Ở đây mới là nơi định nghĩa thật sự.
*/

job_t* jobs = NULL;
int total_jobs = 0;

int completed_jobs = 0;
int total_worker_busy_time = 0;
int worker_count = 0;

policy_t current_policy = POLICY_FIFO;

pthread_mutex_t queue_mutex;
pthread_cond_t job_available;