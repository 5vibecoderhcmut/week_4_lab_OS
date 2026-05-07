# System Design

## 1. Overview

This project implements a mini background job scheduler in C. The system simulates an e-commerce backend where image-processing requests are submitted as background jobs and executed later by a limited worker pool.

The implementation follows the Operating Systems scheduling model:

| OS concept | Project component |
|---|---|
| Process/thread | Background job |
| Ready queue | Job ready queue |
| CPU core | Worker thread |
| CPU scheduler | Job scheduler |
| Dispatcher | Module assigning selected jobs to workers |
| Context activation | Worker starts executing an assigned job |
| Synchronization | Mutex and condition variables |

The main code path is:

```text
CSV workload -> parser -> job array -> arrival simulation -> ready queue
            -> scheduler policy -> dispatcher -> worker thread -> metrics/logs
```

## 2. Source Layout

| Path | Responsibility |
|---|---|
| `src/main.c` | Program orchestration, CLI parsing, job arrival simulation, worker startup/shutdown |
| `src/core/queue.c` | Ready queue implementation using dynamic array of `job_t *` |
| `src/core/scheduler.c` | Policy parsing and `scheduler_get_next_job()` |
| `src/core/dispatcher.c` | Assigns the selected job to a worker |
| `src/core/worker.c` | Worker pool, worker loop, thread wait/wake, job execution |
| `src/policies/fifo.c` | FIFO/FCFS selection |
| `src/policies/sjf.c` | Shortest Job First selection |
| `src/policies/priority.c` | Priority Scheduling selection |
| `src/policies/aging.c` | Aging Priority bonus policy |
| `src/metrics/metrics.c` | Waiting time, turnaround time, throughput, utilization, starvation risk |
| `src/utils/parser.c` | CSV parser |
| `src/utils/time_utils.c` | Scaled time simulation |
| `src/utils/logger.c` | Thread-safe worker logs |
| `src/utils/sync.c` | Mutex and condition variable initialization |
| `src/utils/plot_logs.py` | Generates SVG charts from log summaries |

## 3. Core Data Structures

### 3.1 `job_t`

Defined in `include/job.h`.

```c
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
```

Important fields:

- `arrival_time`: simulated time when the job becomes available.
- `estimated_runtime`: simulated execution duration.
- `priority`: smaller value means higher priority.
- `start_time`: time when a worker starts the job.
- `finish_time`: time when the job completes.
- `sequence_no`: insertion order into the ready queue, used for stable FIFO tie-breaking.

### 3.2 `worker_t`

Defined in `include/worker.h`.

```c
typedef struct {
    int worker_id;
    pthread_t thread;
    void *ctx;
    long busy_time;
} worker_t;
```

Each worker owns one `pthread_t`. Workers share the scheduler context through `ctx`.

### 3.3 `scheduler_context_t`

Defined in `include/common.h`.

```c
typedef struct {
    job_t *jobs;
    int job_count;
    int completed_jobs;
    int worker_count;
    policy_t policy;

    job_queue_t ready_queue;
    pthread_mutex_t mutex;
    pthread_cond_t job_available;
    pthread_cond_t all_done;

    int shutdown;
    long start_wall_ms;
    long total_worker_busy_time;
    long sequence_counter;
} scheduler_context_t;
```

This structure is the main shared state. It contains the job list, ready queue, counters, timing information, and synchronization primitives.

## 4. Runtime Workflow

The program starts in `src/main.c`.

1. Parse command:

```bash
./scheduler <jobs.csv> <policy> <workers>
```

2. Load jobs from CSV using `load_jobs_from_csv()`.
3. Sort jobs by `arrival_time`, then by `job_id`.
4. Initialize:
   - ready queue
   - mutex
   - `job_available` condition variable
   - `all_done` condition variable
5. Create worker threads using `workers_start()`.
6. Main thread simulates job arrivals:
   - sleep until each arrival time
   - push arrived jobs into ready queue
   - broadcast `job_available`
7. Workers wake up and request jobs.
8. Scheduler selects a job according to policy.
9. Dispatcher assigns the job to the worker.
10. Worker logs start, sleeps for simulated runtime, logs finish.
11. Metrics are calculated after all jobs finish.

## 5. Scheduler and Dispatcher Separation

The PDF requires visible separation between scheduler, dispatcher, and worker. This project implements that separation directly.

### Scheduler

Implemented in `src/core/scheduler.c`.

Main function:

```c
job_t *scheduler_get_next_job(scheduler_context_t *ctx);
```

It chooses which job should run next based on the selected policy.

### Dispatcher

Implemented in `src/core/dispatcher.c`.

Main function:

```c
void dispatcher_assign_job(worker_t *worker, job_t *job, int start_time);
```

It assigns the selected job to a worker and updates:

- `job->status`
- `job->start_time`
- `job->assigned_worker_id`

### Worker

Implemented in `src/core/worker.c`.

Main function:

```c
void *worker_loop(void *arg);
```

The worker does not decide policy logic directly. It asks the scheduler for a job, uses the dispatcher to assign it, then executes it.

## 6. Synchronization Design

The project uses:

```c
pthread_mutex_t mutex;
pthread_cond_t job_available;
pthread_cond_t all_done;
```

### Protected Shared Data

The mutex protects:

- ready queue
- job status
- `completed_jobs`
- `shutdown`
- `sequence_counter`
- `total_worker_busy_time`

### Worker Waiting

If the ready queue is empty, a worker blocks:

```c
while (queue_is_empty(&ctx->ready_queue) && !ctx->shutdown) {
    pthread_cond_wait(&ctx->job_available, &ctx->mutex);
}
```

This avoids busy waiting. Workers sleep until the main thread pushes new jobs or requests shutdown.

### Main Thread Wake-up

When jobs arrive, the main thread pushes them into the ready queue and calls:

```c
pthread_cond_broadcast(&ctx->job_available);
```

When the last job finishes, a worker signals:

```c
pthread_cond_signal(&ctx->all_done);
```

The main thread waits on `all_done` until every job completes.

## 7. Scheduling Policies

### FIFO / FCFS

File: `src/policies/fifo.c`

FIFO chooses the job with the smallest `sequence_no`. This means the first job that entered the ready queue is selected first.

Tie-breaking:

```text
sequence_no ascending
```

### SJF

File: `src/policies/sjf.c`

SJF chooses the ready job with the smallest `estimated_runtime`.

Tie-breaking:

```text
estimated_runtime ascending
arrival_time ascending
sequence_no ascending
job_id ascending
```

This is non-preemptive SJF. Once a worker starts a job, the job is not interrupted.

### Priority

File: `src/policies/priority.c`

Priority Scheduling chooses the job with the smallest `priority` value. In this project:

```text
priority = 1 is highest priority
priority = 3 is lower priority
```

Tie-breaking:

```text
priority ascending
arrival_time ascending
sequence_no ascending
job_id ascending
```

### Aging Priority

File: `src/policies/aging.c`

Aging is implemented as a bonus policy to reduce starvation.

Effective priority:

```text
effective_priority = priority - waiting_time / AGING_INTERVAL
```

Because smaller priority is better, a job that waits longer gradually becomes more important.

## 8. Time Simulation

The project uses scaled simulation:

```text
1 simulated time unit = 100 ms
```

Defined in `include/config.h`:

```c
#define DEFAULT_TIME_UNIT_MS 100
```

Execution sleep:

```c
sleep_sim_units(job->estimated_runtime);
```

For deterministic metrics, the finish time is calculated as:

```text
finish_time = start_time + estimated_runtime
```

This avoids small wall-clock timing variations from affecting the reported scheduling metrics.

## 9. Metrics Design

Implemented in `src/metrics/metrics.c`.

The project calculates:

| Metric | Formula |
|---|---|
| Waiting time | `start_time - arrival_time` |
| Turnaround time | `finish_time - arrival_time` |
| Throughput | `total_completed_jobs / total_simulation_time` |
| Worker utilization | `total_worker_busy_time / (worker_count * total_simulation_time)` |
| Starvation risk | jobs with waiting time greater than threshold |

Starvation threshold:

```text
starvation_threshold = 2 * average_runtime
```

This follows the suggestion in the PDF.

## 10. Output Design

Each worker prints start and finish logs:

```text
[time=0] Worker 0 starts Job 1 seller=A runtime=8 priority=2 type=resize
[time=8] Worker 0 finishes Job 1
```

At the end, the program prints:

```text
Policy: SJF
Workers: 4
Total jobs: 20
Total simulation time: 26

Average waiting time: 0.50
Average turnaround time: 3.60
Throughput: 0.769 jobs/unit time
Worker utilization: 59.62%
Starvation threshold: 6.20
Starvation-risk jobs: 0
```

The output satisfies the PDF requirement and adds `Starvation threshold` to make the report easier to explain.

## 11. Design Trade-offs

### Why dynamic array queue?

The scheduler must select jobs by different criteria. FIFO only needs queue order, but SJF, Priority, and Aging need scanning. A dynamic array makes policy selection simple and clear:

```text
scan ready_queue -> find best index -> remove selected job
```

The workload size is small, so O(n) scanning is acceptable.

### Why non-preemptive policies?

The exercise asks for a mini background job scheduler. Once a worker starts an image-processing job, interrupting it would complicate state management. Non-preemptive execution keeps the model close to real background worker systems.

### Why scaled sleep?

Real `sleep(runtime)` would make experiments slow. Scaled simulation keeps the behavior visible while allowing all workloads and policies to run quickly.

### Why include Aging?

Priority Scheduling can delay low-priority jobs. Aging demonstrates a standard OS technique for reducing starvation by gradually increasing the effective priority of waiting jobs.
