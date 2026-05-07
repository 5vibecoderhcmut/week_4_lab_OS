# Code Implementation Plan - Mini Background Job Scheduler in C

## 1. Muc tieu tu file PDF

Xay dung mot chuong trinh C mo phong he thong xu ly background job cho nen tang e-commerce. Moi seller upload anh san pham se tao ra mot job. He thong co so worker gioi han, nen scheduler phai chon job nao duoc giao cho worker dang ranh.

Chuong trinh can:

- Doc danh sach job tu file CSV.
- Mo phong arrival time cua tung job.
- Tao worker pool bang `pthread`.
- Dung `pthread_mutex_t` va `pthread_cond_t` de dong bo queue/job list.
- Tach ro 3 vai tro:
  - Scheduler: quyet dinh job tiep theo theo policy.
  - Dispatcher: giao job da chon cho worker.
  - Worker: chay job, log start/finish, cap nhat trang thai.
- Ho tro it nhat 2 policy:
  - FIFO / FCFS.
  - Them it nhat 1 trong SJF, Priority, Round-Robin simplified, Aging Priority.
- Nen ho tro 3 policy chinh:
  - FIFO.
  - SJF.
  - Priority.
- Tinh va in metrics:
  - Average waiting time.
  - Average turnaround time.
  - Throughput.
  - Worker utilization.
  - Starvation-risk jobs.
- Co workload A/B/C, log output, README, report 4-6 trang.

Repo hien tai da co dung cau truc module, nhung phan lon file `.c`, `.h`, `Makefile`, script dang rong. Plan nay mo ta cach hoan thien code theo dung cau truc san co.

## 2. Cau truc repo hien tai va vai tro tung file

```text
week_4_lab_OS/
├── Makefile
├── README.md
├── structure.txt
├── docs/
│   └── code_implementation_plan.md
├── include/
│   ├── common.h
│   ├── config.h
│   ├── dispatcher.h
│   ├── job.h
│   ├── logger.h
│   ├── metrics.h
│   ├── parser.h
│   ├── queue.h
│   ├── scheduler.h
│   ├── sync.h
│   └── worker.h
├── logs/
├── results/
├── scripts/
│   ├── benchmark.sh
│   └── run_all.sh
├── src/
│   ├── core/
│   │   ├── dispatcher.c
│   │   ├── queue.c
│   │   ├── scheduler.c
│   │   └── worker.c
│   ├── metrics/
│   │   └── metrics.c
│   ├── policies/
│   │   ├── aging.c
│   │   ├── fifo.c
│   │   ├── priority.c
│   │   └── sjf.c
│   ├── utils/
│   │   ├── helpers.c
│   │   ├── logger.c
│   │   └── time_utils.c
│   └── main.c
├── tests/
└── workloads/
    ├── workload_a.csv
    ├── workload_b.csv
    └── workload_c.csv
```

File SJF duoc dat dung ten `src/policies/sjf.c` de khop voi thuat ngu Shortest Job First trong PDF.

## 3. Design tong the

### 3.1 Luong chay chuong trinh

```text
main()
  parse CLI args: ./scheduler <csv_path> <policy> <workers>
  load jobs from CSV
  sort jobs by arrival_time, tie-break by job_id
  init scheduler context
  init mutex + cond var
  create worker threads
  create or run arrival loop
  wait until all jobs completed
  join workers
  calculate metrics
  print summary
  cleanup memory/sync objects
```

### 3.2 Mo phong time

PDF cho 2 option:

- Real-time: `sleep(estimated_runtime)`.
- Scaled simulation: `usleep(estimated_runtime * 100000)`.

Khuyen nghi dung scaled simulation:

- 1 simulated time unit = 100 ms.
- Neu job runtime = 5 thi worker sleep 0.5s.
- Log van in simulated time unit, khong in wall-clock second.

Can co ham chuyen doi:

```c
long now_sim_time(scheduler_context_t *ctx);
void sleep_sim_units(int units);
```

`now_sim_time` tinh tu monotonic clock:

```text
elapsed_ms = current_ms - ctx->start_wall_ms
sim_time = elapsed_ms / TIME_UNIT_MS
```

### 3.3 Mo hinh shared state

Tat ca thread dung chung mot `scheduler_context_t`, gom:

- `job_t *jobs`
- `int job_count`
- `int completed_jobs`
- `policy_t policy`
- `job_queue_t ready_queue`
- `pthread_mutex_t mutex`
- `pthread_cond_t job_available`
- `int shutdown`
- `long start_wall_ms`
- `int worker_count`
- `long total_worker_busy_time`

Moi worker:

- Lock mutex.
- Doi job neu queue rong va chua shutdown.
- Goi scheduler de lay job tot nhat tu ready queue.
- Dispatcher gan job cho worker.
- Unlock mutex.
- Log start.
- Sleep theo runtime.
- Lock mutex.
- Cap nhat finish/status/completed/busy time.
- Signal/broadcast cond var.
- Unlock mutex.

### 3.4 Cach dua job vao ready queue theo arrival_time

Co 2 cach implement.

Option A - main thread lam arrival controller:

```text
main creates workers
for each job sorted by arrival_time:
  sleep until job.arrival_time
  lock
  mark JOB_WAITING
  push job into ready_queue
  broadcast job_available
  unlock
wait until completed_jobs == job_count
shutdown workers
```

Option B - workers tu check job list theo `now_sim_time`.

Khuyen nghi Option A vi:

- Don gian.
- Tach ro arrival simulation.
- Worker chi quan tam ready queue.
- Tranh viec nhieu worker cung scan job list.

## 4. Header design chi tiet

### 4.1 `include/config.h`

Muc dich: cac hang so cau hinh.

Can them:

```c
#ifndef CONFIG_H
#define CONFIG_H

#define MAX_SELLER_ID_LEN 32
#define MAX_JOB_TYPE_LEN 64
#define MAX_LINE_LEN 256
#define DEFAULT_TIME_UNIT_MS 100
#define STARVATION_MULTIPLIER 2.0

#endif
```

Quyet dinh:

- `DEFAULT_TIME_UNIT_MS = 100`: dung scaled simulation nhu PDF khuyen nghi.
- Starvation threshold = `2 * average_runtime`, giong vi du PDF.

### 4.2 `include/job.h`

Muc dich: dinh nghia job va status.

Can them:

```c
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
```

Giai thich field:

- `start_time`: simulated time worker bat dau chay job.
- `finish_time`: simulated time worker chay xong job.
- `assigned_worker_id`: dung cho log/debug.
- `sequence_no`: thu tu vao ready queue, dung tie-break FIFO.

### 4.3 `include/common.h`

Muc dich: enum policy va context chung.

Can tranh circular include. Dinh nghia `policy_t` o day hoac `scheduler.h`. Khuyen nghi o `common.h`.

```c
#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>
#include "job.h"
#include "queue.h"

typedef enum {
    POLICY_FIFO = 0,
    POLICY_SJF,
    POLICY_PRIORITY,
    POLICY_AGING
} policy_t;

typedef struct {
    job_t *jobs;
    int job_count;
    int completed_jobs;
    int next_arrival_index;
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

#endif
```

Neu `queue.h` can `job_t` nhung khong can context, ok. Neu circular include xay ra, dung forward declaration.

### 4.4 `include/queue.h`

Muc dich: queue chua con tro toi `job_t`.

Vi scheduler policy can chon job bat ky trong ready queue, khong nen chi dung linked-list FIFO pop dau. Nen dung dynamic array:

```c
#ifndef QUEUE_H
#define QUEUE_H

#include "job.h"

typedef struct {
    job_t **items;
    int size;
    int capacity;
} job_queue_t;

int queue_init(job_queue_t *queue, int capacity);
void queue_destroy(job_queue_t *queue);
int queue_push(job_queue_t *queue, job_t *job);
job_t *queue_remove_at(job_queue_t *queue, int index);
job_t *queue_peek_at(job_queue_t *queue, int index);
int queue_is_empty(const job_queue_t *queue);

#endif
```

Policy se tim index tot nhat, roi scheduler remove index do.

### 4.5 `include/scheduler.h`

Muc dich: parse policy, lay job tiep theo.

```c
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"

int parse_policy(const char *text, policy_t *out_policy);
const char *policy_to_string(policy_t policy);
job_t *scheduler_get_next_job(scheduler_context_t *ctx);

#endif
```

Ham `scheduler_get_next_job` phai:

- Gia dinh caller da lock mutex.
- Dua vao `ctx->policy`.
- Goi policy-specific selector.
- Remove job khoi ready queue.
- Set status thanh `JOB_RUNNING`.

### 4.6 `include/dispatcher.h`

Muc dich: tach dispatcher theo yeu cau PDF.

```c
#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "common.h"
#include "worker.h"

void dispatcher_assign_job(worker_t *worker, job_t *job, int start_time);

#endif
```

Dispatcher cap nhat:

- `job->assigned_worker_id`
- `job->start_time`
- `job->status = JOB_RUNNING`

### 4.7 `include/worker.h`

Muc dich: dinh nghia worker thread.

```c
#ifndef WORKER_H
#define WORKER_H

#include <pthread.h>

typedef struct {
    int worker_id;
    pthread_t thread;
    void *ctx;
    long busy_time;
} worker_t;

int workers_start(worker_t *workers, int worker_count, void *ctx);
int workers_join(worker_t *workers, int worker_count);
void *worker_loop(void *arg);

#endif
```

`ctx` dung `void *` de tranh include cycle; trong `.c` cast ve `scheduler_context_t *`.

### 4.8 `include/parser.h`

Muc dich: doc CSV workload.

```c
#ifndef PARSER_H
#define PARSER_H

#include "job.h"

int load_jobs_from_csv(const char *path, job_t **out_jobs, int *out_count);
void free_jobs(job_t *jobs);

#endif
```

Parser can:

- Bo header line.
- Validate co du 6 field.
- Validate int fields > hop ly:
  - `job_id > 0`
  - `arrival_time >= 0`
  - `estimated_runtime > 0`
  - `priority > 0`
- Set default:
  - `start_time = -1`
  - `finish_time = -1`
  - `status = JOB_NEW`
  - `assigned_worker_id = -1`

### 4.9 `include/metrics.h`

Muc dich: tinh summary.

```c
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
```

Cong thuc theo PDF:

```text
waiting_time = start_time - arrival_time
turnaround_time = finish_time - arrival_time
throughput = total_completed_jobs / total_simulation_time
worker_utilization = total_worker_busy_time / (workers * total_simulation_time)
starvation_threshold = 2 * average_runtime
```

### 4.10 `include/logger.h`

Muc dich: in log dung format PDF.

```c
#ifndef LOGGER_H
#define LOGGER_H

#include "job.h"

void log_worker_start(int sim_time, int worker_id, const job_t *job);
void log_worker_finish(int sim_time, int worker_id, const job_t *job);
void log_error(const char *fmt, ...);

#endif
```

Format:

```text
[time=0] Worker 0 starts Job 1 seller=A runtime=8 priority=2 type=resize
[time=8] Worker 0 finishes Job 1
```

### 4.11 `include/sync.h`

Muc dich: gom init/destroy sync neu muon tach rieng.

Co the de toi gian:

```c
#ifndef SYNC_H
#define SYNC_H

#include "common.h"

int sync_init(scheduler_context_t *ctx);
void sync_destroy(scheduler_context_t *ctx);

#endif
```

Neu code don gian, co the init mutex/cond trong `main.c` va bo file nay. Nhung vi repo da co `sync.h`, nen nen dung.

## 5. Source file implementation plan

### 5.1 `src/main.c`

Trach nhiem:

- Validate CLI args.
- Load jobs.
- Parse policy.
- Sort jobs theo arrival time.
- Init context.
- Start workers.
- Chay arrival loop.
- Doi all jobs done.
- Shutdown workers.
- Tinh metrics.
- Free resource.

Pseudo-code:

```c
int main(int argc, char **argv) {
    if (argc != 4) {
        print_usage(argv[0]);
        return 1;
    }

    const char *csv_path = argv[1];
    const char *policy_text = argv[2];
    int worker_count = parse_positive_int(argv[3]);

    policy_t policy;
    if (!parse_policy(policy_text, &policy)) return 1;

    job_t *jobs = NULL;
    int job_count = 0;
    if (load_jobs_from_csv(csv_path, &jobs, &job_count) != 0) return 1;

    sort_jobs_by_arrival_then_id(jobs, job_count);

    scheduler_context_t ctx = {0};
    ctx.jobs = jobs;
    ctx.job_count = job_count;
    ctx.worker_count = worker_count;
    ctx.policy = policy;
    ctx.start_wall_ms = monotonic_ms();

    queue_init(&ctx.ready_queue, job_count);
    sync_init(&ctx);

    worker_t *workers = calloc(worker_count, sizeof(worker_t));
    workers_start(workers, worker_count, &ctx);

    run_arrival_loop(&ctx);
    wait_until_all_done(&ctx);
    shutdown_workers(&ctx);

    workers_join(workers, worker_count);

    metrics_t metrics = metrics_calculate(&ctx);
    metrics_print_summary(&ctx, &metrics);

    cleanup...
}
```

Important details:

- Khi job den:
  - Sleep toi dung `arrival_time`.
  - Lock mutex.
  - Set `status = JOB_WAITING`.
  - Set `sequence_no = ctx.sequence_counter++`.
  - Push vao ready queue.
  - `pthread_cond_broadcast(&ctx.job_available)`.
  - Unlock.
- Sau khi push het job:
  - Lock.
  - Neu `completed_jobs < job_count`, wait `all_done`.
  - Set shutdown = 1.
  - Broadcast `job_available`.
  - Unlock.

### 5.2 `src/core/queue.c`

Trach nhiem:

- Dynamic array cho ready queue.
- Push job pointer.
- Remove job tai index bat ky.

Implementation notes:

- `queue_init` allocate `items`.
- `queue_push` double capacity neu full.
- `queue_remove_at`:
  - Lay `items[index]`.
  - Shift cac item sau sang trai.
  - Giam size.
  - Return job pointer.
- Khong lock trong queue.c. Caller tu lock mutex.

### 5.3 `src/core/scheduler.c`

Trach nhiem:

- Convert CLI policy string sang enum.
- Chon job theo policy.
- Goi selector tu `src/policies/*.c`.

Can khai bao internal selector prototype trong `scheduler.c` hoac public trong `scheduler.h`:

```c
int policy_select_fifo(const job_queue_t *queue);
int policy_select_sjf(const job_queue_t *queue);
int policy_select_priority(const job_queue_t *queue);
int policy_select_aging(const job_queue_t *queue, int now);
```

Pseudo-code:

```c
job_t *scheduler_get_next_job(scheduler_context_t *ctx) {
    if (queue_is_empty(&ctx->ready_queue)) return NULL;

    int index = 0;
    switch (ctx->policy) {
        case POLICY_FIFO:
            index = policy_select_fifo(&ctx->ready_queue);
            break;
        case POLICY_SJF:
            index = policy_select_sjf(&ctx->ready_queue);
            break;
        case POLICY_PRIORITY:
            index = policy_select_priority(&ctx->ready_queue);
            break;
        case POLICY_AGING:
            index = policy_select_aging(&ctx->ready_queue, now_sim_time(ctx));
            break;
    }

    return queue_remove_at(&ctx->ready_queue, index);
}
```

Tie-break rule bat buoc de ket qua on dinh:

- FIFO: sequence_no nho nhat.
- SJF: runtime nho nhat, neu bang nhau thi sequence_no nho nhat.
- Priority: priority nho nhat la cao nhat, neu bang nhau thi sequence_no nho nhat.
- Aging: effective priority nho nhat, neu bang nhau thi sequence_no nho nhat.

### 5.4 `src/core/dispatcher.c`

Trach nhiem:

- Cap nhat metadata khi job duoc giao worker.
- The hien separation giua scheduler va dispatcher theo PDF.

Implementation:

```c
void dispatcher_assign_job(worker_t *worker, job_t *job, int start_time) {
    job->status = JOB_RUNNING;
    job->start_time = start_time;
    job->assigned_worker_id = worker->worker_id;
}
```

Khong sleep, khong chon job trong dispatcher.

### 5.5 `src/core/worker.c`

Trach nhiem:

- Tao/join worker threads.
- Worker loop doi job va execute job.

Pseudo-code:

```c
void *worker_loop(void *arg) {
    worker_t *worker = arg;
    scheduler_context_t *ctx = worker->ctx;

    while (1) {
        pthread_mutex_lock(&ctx->mutex);

        while (queue_is_empty(&ctx->ready_queue) && !ctx->shutdown) {
            pthread_cond_wait(&ctx->job_available, &ctx->mutex);
        }

        if (ctx->shutdown && queue_is_empty(&ctx->ready_queue)) {
            pthread_mutex_unlock(&ctx->mutex);
            break;
        }

        job_t *job = scheduler_get_next_job(ctx);
        int start = (int)now_sim_time(ctx);
        dispatcher_assign_job(worker, job, start);

        pthread_mutex_unlock(&ctx->mutex);

        log_worker_start(start, worker->worker_id, job);
        sleep_sim_units(job->estimated_runtime);
        int finish = (int)now_sim_time(ctx);
        log_worker_finish(finish, worker->worker_id, job);

        pthread_mutex_lock(&ctx->mutex);
        job->finish_time = finish;
        job->status = JOB_DONE;
        worker->busy_time += job->estimated_runtime;
        ctx->total_worker_busy_time += job->estimated_runtime;
        ctx->completed_jobs++;
        if (ctx->completed_jobs == ctx->job_count) {
            pthread_cond_signal(&ctx->all_done);
        }
        pthread_mutex_unlock(&ctx->mutex);
    }

    return NULL;
}
```

Can can nhac:

- `finish` theo wall-clock co the lech 1 unit do scheduling OS. De metrics on dinh, co the dat:
  - `finish_time = start_time + estimated_runtime`
  - van sleep real scaled.
- Neu muon log dep va metrics deterministic, khuyen nghi dung `finish_time = start + estimated_runtime`.

Khuyen nghi implementation deterministic:

- `start_time = max(now_sim_time(ctx), job->arrival_time)`.
- `finish_time = start_time + estimated_runtime`.
- Sleep only de demo concurrency, metrics dua tren simulated fields.

### 5.6 `src/policies/fifo.c`

Trach nhiem:

- Chon job vao queue som nhat.

Implementation:

```c
int policy_select_fifo(const job_queue_t *queue) {
    int best = 0;
    for (int i = 1; i < queue->size; i++) {
        if (queue->items[i]->sequence_no < queue->items[best]->sequence_no) {
            best = i;
        }
    }
    return best;
}
```

### 5.7 `src/policies/sjf.c`

Trach nhiem:

- SJF non-preemptive: chon job co `estimated_runtime` nho nhat trong ready queue.

Tie-break:

1. `estimated_runtime` nho hon.
2. `arrival_time` nho hon.
3. `sequence_no` nho hon.
4. `job_id` nho hon.

Implementation:

```c
int policy_select_sjf(const job_queue_t *queue);
```

### 5.8 `src/policies/priority.c`

Trach nhiem:

- Priority Scheduling non-preemptive.
- Theo PDF: so priority nho hon nghia la uu tien cao hon, tru khi nhom dinh nghia nguoc lai.

Quyet dinh trong project:

- `priority = 1` la cao nhat.
- `priority = 3` thap hon.

Tie-break:

1. `priority` nho hon.
2. `arrival_time` nho hon.
3. `sequence_no` nho hon.
4. `job_id` nho hon.

### 5.9 `src/policies/aging.c`

Trach nhiem bonus:

- Giam starvation bang cach job doi cang lau thi effective priority cang cao.

Cong thuc de de giai thich trong report:

```text
waiting = now - arrival_time
aging_boost = waiting / AGING_INTERVAL
effective_priority = priority - aging_boost
```

Voi priority nho la cao:

- Job doi lau se co `effective_priority` nho hon.
- Co the clamp min = 0.

Hang so:

```c
#define AGING_INTERVAL 5
```

Policy nay la bonus, khong bat buoc neu can hoan thanh nhanh. Neu implement, README/report can noi ro.

### 5.10 `src/metrics/metrics.c`

Trach nhiem:

- Tinh summary sau khi tat ca job done.
- In dung format PDF.

Pseudo-code:

```c
metrics_t metrics_calculate(const scheduler_context_t *ctx) {
    int min_arrival = min(job.arrival_time);
    int max_finish = max(job.finish_time);
    int total_time = max_finish - min_arrival;
    if (total_time <= 0) total_time = 1;

    double total_wait = 0;
    double total_turnaround = 0;
    double total_runtime = 0;

    for each job:
      wait = job.start_time - job.arrival_time
      turnaround = job.finish_time - job.arrival_time
      total_wait += wait
      total_turnaround += turnaround
      total_runtime += job.estimated_runtime

    avg_runtime = total_runtime / job_count
    threshold = 2 * avg_runtime
    starvation_count = count(wait > threshold)

    throughput = job_count / total_time
    utilization = total_runtime / (worker_count * total_time) * 100
}
```

Can chu y:

- Neu worker_count lon va jobs den muon, utilization se phan anh ca thoi gian idle vi chua co job.
- Co the dung `total_worker_busy_time` thay `total_runtime`; hai gia tri nen bang nhau neu moi job chay dung mot lan.

Output expected:

```text
Policy: FIFO
Workers: 4
Total jobs: 20
Total simulation time: 32

Average waiting time: 6.25
Average turnaround time: 11.40
Throughput: 0.625 jobs/unit time
Worker utilization: 78.12%
Starvation-risk jobs: 3
```

### 5.11 `src/utils/logger.c`

Trach nhiem:

- In log thread-safe.
- Dung mot `pthread_mutex_t log_mutex` static de tranh log bi xen dong.

Implementation:

```c
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
```

`log_worker_start` va `log_worker_finish` lock/unlock quanh `printf`.

### 5.12 `src/utils/time_utils.c`

Trach nhiem:

- Lay monotonic ms.
- Sleep simulated unit.
- Convert wall time sang simulated time.

Can dung:

```c
#include <time.h>
#include <unistd.h>
```

Functions:

```c
long monotonic_ms(void);
void sleep_ms(long ms);
void sleep_sim_units(int units);
long elapsed_sim_time(long start_wall_ms);
```

### 5.13 `src/utils/helpers.c`

Trach nhiem:

- Parse positive int.
- Sort jobs.
- Print usage.
- String trim neu can.

Functions:

```c
int parse_positive_int(const char *text, int *out);
void sort_jobs_by_arrival_then_id(job_t *jobs, int count);
void print_usage(const char *program);
```

### 5.14 Parser source file bi thieu

Repo co `include/parser.h` nhung chua co `src/utils/parser.c` hay `src/core/parser.c`.

Can them file moi:

```text
src/utils/parser.c
```

Ly do:

- Parser la utility, khong thuoc core scheduler.
- `Makefile` se compile `src/utils/*.c`.

CSV format theo PDF:

```csv
job_id,seller_id,arrival_time,estimated_runtime,priority,job_type
1,A,0,8,2,resize
```

Parser implementation:

- Mo file bang `fopen`.
- Doc bang `fgets`.
- Skip line dau neu chua header.
- Dung `strtok_r` theo dau phay.
- Copy string bang `snprintf` hoac `strncpy` co dam bao null-terminated.
- Neu line loi, return non-zero va in message ro.

## 6. Makefile plan

`Makefile` hien dang rong. Can them:

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread -Iinclude
TARGET = scheduler

SRC = \
	src/main.c \
	src/core/dispatcher.c \
	src/core/queue.c \
	src/core/scheduler.c \
	src/core/worker.c \
	src/metrics/metrics.c \
	src/policies/fifo.c \
	src/policies/sjf.c \
	src/policies/priority.c \
	src/policies/aging.c \
	src/utils/helpers.c \
	src/utils/logger.c \
	src/utils/parser.c \
	src/utils/time_utils.c

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
```


## 7. Scripts plan

### 7.1 `scripts/run_all.sh`

Muc dich:

- Build.
- Chay 9 case: A/B/C x FIFO/SJF/Priority voi 4 workers.
- Ghi log vao `logs/`.

Cases:

```text
workload_a.csv fifo 4 -> logs/a_fifo_4.txt
workload_a.csv sjf 4 -> logs/a_sjf_4.txt
workload_a.csv priority 4 -> logs/a_priority_4.txt
workload_b.csv fifo 4 -> logs/b_fifo_4.txt
workload_b.csv sjf 4 -> logs/b_sjf_4.txt
workload_b.csv priority 4 -> logs/b_priority_4.txt
workload_c.csv fifo 4 -> logs/c_fifo_4.txt
workload_c.csv sjf 4 -> logs/c_sjf_4.txt
workload_c.csv priority 4 -> logs/c_priority_4.txt
```

### 7.2 `scripts/benchmark.sh`

Muc dich:

- Chay nhieu worker counts: 2, 4, 8.
- Phuc vu bonus "Compare Different Worker Counts".

Cases:

```text
for workload in a b c
for policy in fifo sjf priority
for workers in 2 4 8
```

Ghi logs:

```text
logs/a_fifo_2.txt
logs/a_fifo_4.txt
logs/a_fifo_8.txt
...
```

Optional:

- Parse summary lines thanh CSV `results/summary.csv`.

## 8. Workload mapping theo PDF

### 8.1 `workloads/workload_a.csv` - Balanced Workload

Hien co 20 jobs:

- Runtime chu yeu 4-7.
- Priority mixed 1-3.
- Arrival time tu 0 den 10.

Muc dich:

- Test xem FIFO co du tot khi job runtime gan nhau.
- SJF co the khong cai thien qua nhieu.
- Priority co the giam waiting time cho priority cao nhung khong nhat thiet giam average.

### 8.2 `workloads/workload_b.csv` - Mixed Short and Long Jobs

Hien co 20 jobs:

- Nhieu job ngan runtime 1-2.
- Co job dai runtime 15 va 19.

Muc dich:

- Quan sat convoy effect cua FIFO neu job dai chay truoc.
- SJF nen giam average waiting/turnaround cho job ngan.
- Job dai co the doi lau hon voi SJF.

### 8.3 `workloads/workload_c.csv` - Priority-Sensitive Workload

Hien co 20 jobs:

- Job dau priority 3 den truoc.
- Tu job 11 tro di nhieu job priority 1 den muon hon.

Muc dich:

- Priority Scheduling uu tien job quan trong den sau.
- Co the lam job priority thap doi lau.
- Dung de thao luan starvation risk.

## 9. CLI behavior

Command theo PDF:

```bash
./scheduler workloads/workload_a.csv fifo 4
./scheduler workloads/workload_a.csv sjf 4
./scheduler workloads/workload_a.csv priority 4
```

Supported policy strings:

- `fifo`
- `fcfs` alias cua `fifo`
- `sjf`
- `priority`
- `aging` optional bonus

Neu input sai, in:

```text
Usage: ./scheduler <jobs.csv> <policy> <workers>
Policies: fifo, sjf, priority, aging
Example: ./scheduler workloads/workload_a.csv fifo 4
```

Exit code:

- `0`: thanh cong.
- `1`: invalid arguments, parse CSV fail, thread init fail, memory fail.

## 10. Synchronization rules

Shared data phai duoc bao ve boi mutex:

- `ready_queue`
- `completed_jobs`
- `shutdown`
- `job.status`
- `job.start_time`
- `job.finish_time`
- `ctx.total_worker_busy_time`
- `ctx.sequence_counter`

Condition variables:

- `job_available`:
  - Worker wait khi ready queue rong.
  - Main broadcast khi co job moi den.
  - Main broadcast khi shutdown.
- `all_done`:
  - Main wait den khi `completed_jobs == job_count`.
  - Worker signal khi hoan thanh job cuoi.

Nguyen tac:

- Khong giu mutex trong luc `sleep_sim_units`.
- Worker chi lock luc chon job va cap nhat shared status.
- Queue functions khong tu lock, caller phai lock.
- Logger co mutex rieng de tranh log xen nhau.

## 11. Metrics va expected report table

Sau khi chay `scripts/run_all.sh`, can dien bang:

```text
Workload | Policy   | Workers | Avg waiting | Avg turnaround | Throughput | Utilization | Starvation-risk jobs
A        | FIFO     | 4       |             |                |            |             |
A        | SJF      | 4       |             |                |            |             |
A        | Priority | 4       |             |                |            |             |
B        | FIFO     | 4       |             |                |            |             |
B        | SJF      | 4       |             |                |            |             |
B        | Priority | 4       |             |                |            |             |
C        | FIFO     | 4       |             |                |            |             |
C        | SJF      | 4       |             |                |            |             |
C        | Priority | 4       |             |                |            |             |
```

Neu lam bonus worker counts, them bang:

```text
Workload | Policy | Workers | Avg waiting | Avg turnaround | Throughput | Utilization
A        | FIFO   | 2       |             |                |            |
A        | FIFO   | 4       |             |                |            |
A        | FIFO   | 8       |             |                |            |
```

## 12. README plan

`README.md` can co cac muc:

1. Project title.
2. Problem summary.
3. OS concept mapping.
4. Project structure.
5. Build instructions.
6. Run instructions.
7. Supported policies.
8. Workloads.
9. Metrics definitions.
10. Scripts.
11. Example output.
12. Notes on time simulation.

Example build:

```bash
make
```

Example run:

```bash
./scheduler workloads/workload_a.csv fifo 4
./scheduler workloads/workload_b.csv sjf 4
./scheduler workloads/workload_c.csv priority 4
```

## 13. Testing plan

### 13.1 Build test

```bash
make clean
make
```

Pass criteria:

- Build thanh cong.
- Khong co warning voi `-Wall -Wextra`.

### 13.2 Basic run test

```bash
./scheduler workloads/workload_a.csv fifo 4
```

Pass criteria:

- In log worker start/finish.
- In summary cuoi run.
- Total jobs = 20.
- Khong deadlock.
- Program exit code 0.

### 13.3 Policy test

Chay:

```bash
./scheduler workloads/workload_a.csv fifo 4
./scheduler workloads/workload_a.csv sjf 4
./scheduler workloads/workload_a.csv priority 4
```

Pass criteria:

- FIFO chon theo thu tu job vao ready queue.
- SJF uu tien runtime nho hon trong cac job da arrival.
- Priority uu tien priority number nho hon trong cac job da arrival.

### 13.4 Worker count test

```bash
./scheduler workloads/workload_b.csv fifo 2
./scheduler workloads/workload_b.csv fifo 4
./scheduler workloads/workload_b.csv fifo 8
```

Pass criteria:

- So worker log khong vuot qua configured worker count.
- Metrics utilization thay doi hop ly.
- Khong co race/deadlock.

### 13.5 CSV error test

Tao file loi trong `tests/invalid.csv`:

```csv
job_id,seller_id,arrival_time,estimated_runtime,priority,job_type
1,A,0,0,2,resize
bad,line
```

Pass criteria:

- Parser bao loi ro.
- Program exit non-zero.

### 13.6 Thread safety test

Chay lap nhieu lan:

```bash
for i in 1 2 3 4 5; do ./scheduler workloads/workload_c.csv priority 4; done
```

Pass criteria:

- Khong crash.
- Khong treo.
- Total completed jobs luon = 20.

## 14. Implementation milestones

### Milestone 1 - Core types and build

Files:

- `include/config.h`
- `include/job.h`
- `include/queue.h`
- `include/common.h`
- `Makefile`

Done when:

- `make` co the compile cac file rong hoac minimal stubs.
- Type definitions khong bi circular include.

### Milestone 2 - Parser and helpers

Files:

- `include/parser.h`
- `src/utils/parser.c`
- `src/utils/helpers.c`

Done when:

- Doc du 20 jobs tu moi workload.
- Validate CSV format.
- Sort theo arrival time.

### Milestone 3 - Queue and scheduler policies

Files:

- `src/core/queue.c`
- `src/core/scheduler.c`
- `src/policies/fifo.c`
- `src/policies/sjf.c`
- `src/policies/priority.c`

Done when:

- Unit/manual test selector cho FIFO/SJF/Priority dung.
- `parse_policy` ho tro `fifo`, `sjf`, `priority`.

### Milestone 4 - Worker pool and dispatcher

Files:

- `include/worker.h`
- `include/dispatcher.h`
- `include/sync.h`
- `src/core/worker.c`
- `src/core/dispatcher.c`

Done when:

- Worker threads duoc tao va join.
- Worker doi cond var khi khong co job.
- Worker execute job va update status.
- Khong giu mutex trong luc sleep.

### Milestone 5 - Main orchestration

Files:

- `src/main.c`

Done when:

- CLI chay duoc:
  - `./scheduler workloads/workload_a.csv fifo 4`
  - `./scheduler workloads/workload_b.csv sjf 4`
  - `./scheduler workloads/workload_c.csv priority 4`
- Log start/finish dung format.
- Program ket thuc sau khi completed all jobs.

### Milestone 6 - Metrics

Files:

- `include/metrics.h`
- `src/metrics/metrics.c`

Done when:

- Summary in day du fields trong PDF.
- Cong thuc waiting/turnaround/throughput/utilization/starvation dung.

### Milestone 7 - Scripts and logs

Files:

- `scripts/run_all.sh`
- `scripts/benchmark.sh`
- `logs/*.txt`
- `results/summary.csv` optional

Done when:

- `scripts/run_all.sh` tao du 9 log.
- `scripts/benchmark.sh` tao logs cho 2/4/8 workers neu lam bonus.

### Milestone 8 - README and report support

Files:

- `README.md`
- `docs/report_outline.md` optional

Done when:

- README huong dan compile/run ro rang.
- Co bang ket qua de dua vao report.

## 15. Rubric mapping

### Correct worker pool implementation - 20%

Evidence trong code:

- `worker_t` trong `include/worker.h`.
- `workers_start`, `workers_join`, `worker_loop` trong `src/core/worker.c`.
- Worker lap lai wait -> get job -> execute -> update -> request next job.

### Correct synchronization - 15%

Evidence trong code:

- `pthread_mutex_t mutex`.
- `pthread_cond_t job_available`.
- `pthread_cond_t all_done`.
- Queue/status/counter duoc bao ve bang mutex.

### At least two scheduling policies - 25%

Evidence trong code:

- `src/policies/fifo.c`.
- `src/policies/sjf.c`.
- `src/policies/priority.c`.
- `scheduler_get_next_job` dispatch theo policy.

### Metrics and experiment design - 20%

Evidence:

- `src/metrics/metrics.c`.
- `workloads/workload_a.csv`.
- `workloads/workload_b.csv`.
- `workloads/workload_c.csv`.
- `logs/*.txt`.
- `results/summary.csv` optional.

### Report quality and OS-level discussion - 15%

Evidence:

- README/report giai thich:
  - Scheduler/dispatcher/worker mapping voi OS.
  - Ready queue.
  - Synchronization.
  - Trade-off giua FIFO, SJF, Priority.
  - Starvation.

### Code readability and README - 5%

Evidence:

- File chia module ro.
- Ten ham ro.
- README co command compile/run.
- Khong gom tat ca code vao mot file.

## 16. Risk list va cach tranh loi

### Deadlock

Nguyen nhan:

- Worker wait cond var nhung main khong broadcast.
- Main wait all_done nhung worker khong signal khi job cuoi xong.

Phong tranh:

- Broadcast `job_available` moi lan push job va khi shutdown.
- Signal `all_done` khi `completed_jobs == job_count`.
- Luon dung while loop quanh `pthread_cond_wait`.

### Race condition

Nguyen nhan:

- Nhieu worker cung remove job khoi queue.
- Update `completed_jobs` khong lock.

Phong tranh:

- Lock mutex trong scheduler_get_next_job.
- Lock mutex khi update completed/status.

### Metrics sai do time wall-clock

Nguyen nhan:

- OS scheduling lam `now_sim_time` lech.
- `usleep` khong chinh xac tuyet doi.

Phong tranh:

- Dung simulated fields:
  - `finish_time = start_time + estimated_runtime`.
  - `total_worker_busy_time += estimated_runtime`.

### Starvation threshold khong ro

Nguyen nhan:

- PDF cho phep nhom tu dinh nghia threshold.

Phong tranh:

- Ghi ro trong README/report:
  - `starvation_threshold = 2 * average_runtime`.

### Priority direction khong ro

Nguyen nhan:

- PDF noi smaller number means higher priority unless defined otherwise.

Phong tranh:

- Chon:
  - `priority=1` cao nhat.
  - `priority=3` thap hon.
- Ghi ro trong README va comments.

## 17. Definition of done

Project duoc coi la xong phan code khi:

- `make clean && make` thanh cong khong warning.
- Chay duoc 9 command:

```bash
./scheduler workloads/workload_a.csv fifo 4
./scheduler workloads/workload_a.csv sjf 4
./scheduler workloads/workload_a.csv priority 4
./scheduler workloads/workload_b.csv fifo 4
./scheduler workloads/workload_b.csv sjf 4
./scheduler workloads/workload_b.csv priority 4
./scheduler workloads/workload_c.csv fifo 4
./scheduler workloads/workload_c.csv sjf 4
./scheduler workloads/workload_c.csv priority 4
```

- Moi run:
  - Khong deadlock.
  - Total jobs = 20.
  - Co log start/finish.
  - Co summary metrics.
- `scripts/run_all.sh` tao du log cho 9 run.
- README co huong dan compile/run.
- Code co tach ro:
  - Scheduler.
  - Dispatcher.
  - Worker.
  - Queue.
  - Metrics.
  - Policies.

## 18. De xuat thu tu lam viec cho nhom

Student 1 - Parser and workload:

- `include/parser.h`
- `src/utils/parser.c`
- Kiem tra 3 workload CSV.

Student 2 - Queue and synchronization:

- `include/queue.h`
- `src/core/queue.c`
- `include/sync.h`
- Sync init/destroy.

Student 3 - Scheduler policies:

- `include/scheduler.h`
- `src/core/scheduler.c`
- `src/policies/fifo.c`
- `src/policies/sjf.c`
- `src/policies/priority.c`

Student 4 - Worker and dispatcher:

- `include/worker.h`
- `include/dispatcher.h`
- `src/core/worker.c`
- `src/core/dispatcher.c`

Student 5 - Metrics, scripts, docs:

- `include/metrics.h`
- `src/metrics/metrics.c`
- `scripts/run_all.sh`
- `scripts/benchmark.sh`
- `README.md`
- Ket qua log va bang comparison.

Tat ca thanh vien can review `main.c` vi day la noi ghep toan bo system.
