# Mini Background Job Scheduler in C

This project implements the Operating Systems Lab 4 group exercise: a mini background job scheduler for e-commerce image-processing jobs.

The program reads jobs from CSV, simulates job arrivals, schedules ready jobs with a worker pool, and prints scheduling metrics.

## Build

```bash
make
```

Clean build artifacts:

```bash
make clean
```

## Run

```bash
./scheduler <jobs.csv> <policy> <workers>
```

Examples:

```bash
./scheduler workloads/workload_a.csv fifo 4
./scheduler workloads/workload_b.csv sjf 4
./scheduler workloads/workload_c.csv priority 4
```

Supported policies:

- `fifo` or `fcfs`: first job that entered the ready queue runs first.
- `sjf`: non-preemptive shortest job first.
- `priority`: non-preemptive priority scheduling, where smaller priority number means higher priority.
- `aging`: bonus aging-priority policy to reduce starvation.

## Time Simulation

The program uses scaled simulation:

```text
1 simulated time unit = 100 ms
```

For example, a job with `estimated_runtime=5` sleeps for about 0.5 seconds.

## Workloads

- `workload_a.csv`: balanced workload, most runtimes are similar.
- `workload_b.csv`: mixed short and long jobs, useful for observing FIFO convoy effect and SJF behavior.
- `workload_c.csv`: priority-sensitive workload, high-priority jobs arrive later.

## Scripts

Run the required 9 experiments:

```bash
./scripts/run_all.sh
```

Run 2/4/8 worker comparisons:

```bash
./scripts/benchmark.sh
```

Logs are written to `logs/`.

The required 4-worker comparison table is written to:

```text
results/comparison_4_workers.csv
```

The 2/4/8-worker benchmark table is written to:

```text
results/summary.csv
```

## Metrics

The summary reports:

- Waiting time: `start_time - arrival_time`
- Turnaround time: `finish_time - arrival_time`
- Throughput: `total_completed_jobs / total_simulation_time`
- Worker utilization: `total_worker_busy_time / (workers * total_simulation_time)`
- Starvation risk: jobs whose waiting time is greater than `2 * average_runtime`

## OS Mapping

- Background job: process/thread to be scheduled.
- Ready queue: OS ready queue.
- Worker thread: CPU core.
- Scheduler: selects the next ready job.
- Dispatcher: assigns the selected job to a worker.
- Mutex and condition variable: protect shared queue/status and block workers when no job is ready.
