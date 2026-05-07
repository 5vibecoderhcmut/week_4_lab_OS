#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <pthread.h>

/*
    ============================================================
    COMMON HEADER FILE
    ============================================================

    File này là hợp đồng chung giữa các Student.

    Mọi file .c nên include file này:
        #include "scheduler.h"

    File này chứa:
    - struct job_t
    - struct worker_t
    - enum status/policy
    - biến global dạng extern
    - prototype các hàm
*/

#define MAX_SELLER_ID 32
#define MAX_JOB_TYPE 64

/*
    1 đơn vị thời gian mô phỏng = 100000 microseconds = 0.1 giây thật.

    Ví dụ:
        estimated_runtime = 5
        => worker ngủ 5 * 0.1 = 0.5 giây thật
*/
#define TIME_UNIT_US 100000

typedef enum { JOB_WAITING, JOB_RUNNING, JOB_DONE } job_status_t;

typedef enum { POLICY_FIFO, POLICY_SJF, POLICY_PRIORITY } policy_t;

typedef struct {
    int job_id;
    char seller_id[MAX_SELLER_ID];
    int arrival_time;
    int estimated_runtime;
    int priority;
    char job_type[MAX_JOB_TYPE];

    int start_time;
    int finish_time;

    job_status_t status;
} job_t;

typedef struct {
    int worker_id;
    pthread_t thread;
} worker_t;

/*
    ============================================================
    GLOBAL VARIABLES
    ============================================================

    Ở đây chỉ khai báo extern.
    Biến thật sự được định nghĩa trong global.c.
*/

extern job_t* jobs;
extern int total_jobs;

extern int completed_jobs;
extern int total_worker_busy_time;
extern int worker_count;

extern policy_t current_policy;

extern pthread_mutex_t queue_mutex;
extern pthread_cond_t job_available;

/*
    ============================================================
    INPUT READER
    ============================================================
*/

int load_jobs_from_csv(const char* filename);
void free_jobs(void);

/*
    ============================================================
    POLICY PARSER
    ============================================================
*/

int parse_policy(const char* policy_str, policy_t* policy);
const char* policy_to_string(policy_t policy);

/*
    ============================================================
    SCHEDULER POLICY
    ============================================================
*/

job_t* scheduler_get_next_job(policy_t policy);

/*
    ============================================================
    WORKER SYSTEM - STUDENT 2
    ============================================================
*/

int get_simulation_time(void);

void init_sync_objects(void);
void destroy_sync_objects(void);

void init_jobs_status(void);

worker_t* create_worker_pool(int count);
void start_worker_pool(worker_t* workers, int count);
void join_worker_pool(worker_t* workers, int count);
void notify_workers(void);

void* worker_loop(void* arg);

void run_worker_system(int count);

/*
    ============================================================
    METRICS
    ============================================================
*/

void print_summary(policy_t policy, int worker_count);

#endif