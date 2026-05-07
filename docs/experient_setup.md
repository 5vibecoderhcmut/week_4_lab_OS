# Experiment Setup

This document explains the experiment setup, workloads, metrics, commands, and result interpretation for the current implementation.

Note: the filename is `experient_setup.md` to match the requested file name. The content describes the experiment setup.

## 1. Experiment Goal

The goal is to compare different scheduling policies for a mini background job scheduler:

- FIFO / FCFS
- SJF
- Priority Scheduling
- Aging Priority bonus policy

The comparison focuses on:

- average waiting time
- average turnaround time
- throughput
- worker utilization
- starvation-risk jobs

The experiment answers these questions from the PDF:

- Which policy gives the lowest average waiting time?
- Which policy gives the lowest average turnaround time?
- Which policy gives the best throughput?
- Does the best policy change across workloads?
- Does Priority Scheduling cause starvation?
- Does SJF improve performance for short jobs?
- Does adding more workers always improve performance?

## 2. Environment

The project is written in C and built with `gcc`.

Build command:

```bash
make clean
make
```

Compiler flags from `Makefile`:

```text
-std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -O2 -pthread -Iinclude
```

The `-pthread` flag is required because the implementation uses:

- `pthread_create`
- `pthread_join`
- `pthread_mutex_t`
- `pthread_cond_t`

## 3. Time Simulation Method

The experiment uses scaled simulation.

```text
1 simulated time unit = 100 ms
```

This is defined in `include/config.h`:

```c
#define DEFAULT_TIME_UNIT_MS 100
```

For example:

```text
estimated_runtime = 5
actual sleep time = 5 * 100 ms = 0.5 seconds
```

The report should state that scaled simulation was used instead of real-time `sleep(runtime)`.

## 4. Workloads

The project uses three workload files from `workloads/`.

### Workload A: Balanced Workload

File:

```text
workloads/workload_a.csv
```

Characteristics:

- 20 jobs.
- Runtime values are mostly similar.
- Priorities are mixed.
- Arrival times range from 0 to 10.

Purpose:

- Test whether FIFO is already reasonable when most jobs have similar runtimes.
- Compare whether SJF and Priority provide meaningful improvement.

### Workload B: Mixed Short and Long Jobs

File:

```text
workloads/workload_b.csv
```

Characteristics:

- 20 jobs.
- Many short jobs with runtime 1 or 2.
- Two long jobs with runtime 15 and 19.

Purpose:

- Observe the convoy effect under FIFO.
- Check whether SJF reduces waiting time for short jobs.
- Observe whether long jobs are delayed by SJF.

### Workload C: Priority-Sensitive Workload

File:

```text
workloads/workload_c.csv
```

Characteristics:

- 20 jobs.
- Earlier jobs mostly have lower priority.
- Later jobs include many high-priority jobs.

Purpose:

- Test whether Priority Scheduling improves response for important jobs.
- Observe starvation risk for lower-priority jobs.
- Compare Priority with Aging.

## 5. Policies Compared

### FIFO

Run name:

```text
fifo
```

FIFO chooses the job that entered the ready queue first.

### SJF

Run name:

```text
sjf
```

SJF chooses the ready job with the shortest estimated runtime.

### Priority

Run name:

```text
priority
```

Priority chooses the job with the smallest priority number.

```text
priority = 1 is highest priority
```

### Aging Priority

Run name:

```text
aging
```

Aging adjusts effective priority as jobs wait:

```text
effective_priority = priority - waiting_time / AGING_INTERVAL
```

This policy is included as a bonus comparison.

## 6. Required 4-Worker Experiment

The main comparison uses 4 workers.

Script:

```bash
./scripts/run_all.sh
```

The script runs 12 cases:

| Workload | Policies | Workers |
|---|---|---|
| A | FIFO, SJF, Priority, Aging | 4 |
| B | FIFO, SJF, Priority, Aging | 4 |
| C | FIFO, SJF, Priority, Aging | 4 |

Outputs:

```text
logs/*_4.txt
results/comparison_4_workers.csv
```

## 7. Worker Count Benchmark

The extended benchmark compares 2, 4, and 8 workers.

Script:

```bash
./scripts/benchmark.sh
```

The script runs 36 cases:

```text
3 workloads * 4 policies * 3 worker counts = 36 runs
```

Outputs:

```text
logs/*.txt
results/summary.csv
```

This benchmark is useful for the bonus discussion about whether adding more workers always improves performance.

## 8. Chart Generation

Charts are generated from logs using:

```bash
python3 src/utils/plot_logs.py --logs logs --out results/charts
```

Generated files:

```text
results/charts/avg_waiting_4_workers.svg
results/charts/avg_turnaround_4_workers.svg
results/charts/throughput_4_workers.svg
results/charts/utilization_4_workers.svg
results/charts/starvation_4_workers.svg
results/charts/worker_scaling_a.svg
results/charts/worker_scaling_b.svg
results/charts/worker_scaling_c.svg
results/charts/index.html
```

The charts use linear scale.

## 9. Metrics

The project calculates metrics after all jobs finish.

| Metric | Formula |
|---|---|
| Waiting time | `start_time - arrival_time` |
| Turnaround time | `finish_time - arrival_time` |
| Throughput | `total_completed_jobs / total_simulation_time` |
| Worker utilization | `total_worker_busy_time / (workers * total_simulation_time)` |
| Starvation-risk jobs | jobs whose waiting time exceeds threshold |

Starvation threshold:

```text
starvation_threshold = 2 * average_runtime
```

This threshold is printed in each log summary.

## 10. 4-Worker Results

Source file:

```text
results/comparison_4_workers.csv
```

Current 4-worker results:

| Workload | Policy | Avg waiting | Avg turnaround | Throughput | Utilization | Starvation-risk |
|---|---:|---:|---:|---:|---:|---:|
| A | FIFO | 7.60 | 13.25 | 0.625 | 88.28% | 7 |
| A | SJF | 6.65 | 12.30 | 0.625 | 88.28% | 5 |
| A | Priority | 7.45 | 13.10 | 0.645 | 91.13% | 7 |
| A | Aging | 7.50 | 13.15 | 0.667 | 94.17% | 6 |
| B | FIFO | 0.60 | 3.70 | 0.769 | 59.62% | 0 |
| B | SJF | 0.50 | 3.60 | 0.769 | 59.62% | 0 |
| B | Priority | 0.70 | 3.80 | 0.769 | 59.62% | 0 |
| B | Aging | 0.70 | 3.80 | 0.769 | 59.62% | 0 |
| C | FIFO | 1.85 | 6.65 | 0.690 | 82.76% | 0 |
| C | SJF | 1.70 | 6.50 | 0.645 | 77.42% | 0 |
| C | Priority | 1.80 | 6.60 | 0.690 | 82.76% | 1 |
| C | Aging | 1.80 | 6.60 | 0.690 | 82.76% | 1 |

## 11. Result Analysis

### Average waiting time

SJF gives the lowest average waiting time in all three 4-worker workloads:

- Workload A: SJF = 6.65
- Workload B: SJF = 0.50
- Workload C: SJF = 1.70

This is expected because SJF gives short jobs a chance to finish earlier.

### Average turnaround time

SJF also gives the lowest average turnaround time in all three 4-worker workloads:

- Workload A: SJF = 12.30
- Workload B: SJF = 3.60
- Workload C: SJF = 6.50

The improvement is strongest when there are short jobs waiting behind longer jobs.

### Throughput

Throughput depends on total simulation time:

- Workload A: Aging is highest at 0.667 jobs/unit time.
- Workload B: all policies are equal at 0.769 jobs/unit time.
- Workload C: FIFO, Priority, and Aging are equal at 0.690 jobs/unit time.

This shows that the policy with the best waiting time is not always the policy with the best throughput.

### Worker utilization

Utilization is high when workers stay busy for most of the simulation time.

In Workload A with 4 workers:

- Aging has the highest utilization: 94.17%.
- Priority is next: 91.13%.
- FIFO and SJF are both 88.28%.

In Workload B, utilization is lower with 4 workers because many jobs are very short, so workers become idle sooner.

### Starvation risk

Workload A has the highest starvation-risk count:

- FIFO: 7
- SJF: 5
- Priority: 7
- Aging: 6

Workload B has no starvation risk with 4 workers.

Workload C has 1 starvation-risk job under Priority and Aging. This happens because priority-sensitive scheduling can delay some jobs when important jobs arrive later.

### Worker count effect

From `results/summary.csv`, increasing workers from 2 to 4 generally reduces waiting time and starvation risk significantly.

Examples:

- Workload A FIFO waiting time: 21.55 with 2 workers -> 7.60 with 4 workers -> 1.40 with 8 workers.
- Workload C FIFO waiting time: 13.20 with 2 workers -> 1.85 with 4 workers -> 0.00 with 8 workers.

However, utilization often decreases when workers increase to 8:

- Workload B FIFO utilization: 96.88% with 2 workers -> 59.62% with 4 workers -> 31.00% with 8 workers.

This means adding workers improves waiting time but does not always improve resource efficiency.

## 12. Recommended Report Interpretation

For a real backend job-processing system:

- SJF is good when the goal is to minimize average waiting and turnaround time.
- Priority is useful when important jobs must be served sooner.
- Aging is useful when priority scheduling may starve lower-priority jobs.
- FIFO is simple and fair by arrival order, but it can perform worse when short jobs wait behind longer jobs.

The best policy depends on the system goal:

| Goal | Recommended policy |
|---|---|
| Simplicity and fairness by arrival order | FIFO |
| Lowest average waiting time | SJF |
| Serve important jobs first | Priority |
| Priority with starvation mitigation | Aging |

For this project, SJF is the strongest policy for average waiting and turnaround time in the 4-worker experiments, while Aging is useful as a bonus policy to discuss starvation mitigation.
