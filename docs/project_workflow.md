# Project Workflow

This document provides Markdown/Mermaid diagrams for the Mini Background Job Scheduler project.

## 1. Runtime Workflow

```mermaid
flowchart TD
    A["Start program"] --> B["Parse CLI arguments<br/>CSV path, policy, worker count"]
    B --> C["Load jobs from CSV"]
    C --> D["Sort jobs by arrival_time and job_id"]
    D --> E["Initialize scheduler context"]
    E --> F["Initialize ready queue, mutex, condition variables"]
    F --> G["Create worker pool"]
    G --> H["Main thread simulates job arrivals"]

    H --> I{"Job arrival time reached?"}
    I -- "No" --> H
    I -- "Yes" --> J["Push arrived jobs into ready queue"]
    J --> K["Broadcast job_available"]

    K --> L["Worker wakes up"]
    L --> M["Scheduler selects next job by policy"]
    M --> N["Dispatcher assigns job to worker"]
    N --> O["Worker logs start"]
    O --> P["Worker sleeps for estimated_runtime"]
    P --> Q["Worker logs finish"]
    Q --> R["Update job status, finish_time, counters"]

    R --> S{"All jobs completed?"}
    S -- "No" --> L
    S -- "Yes" --> T["Shutdown workers"]
    T --> U["Join worker threads"]
    U --> V["Calculate metrics"]
    V --> W["Print summary"]
    W --> X["End program"]
```

## 2. Module Workflow

```mermaid
flowchart LR
    CLI["src/main.c<br/>Program orchestration"] --> Parser["src/utils/parser.c<br/>CSV parser"]
    CLI --> Sync["src/utils/sync.c<br/>Mutex + condition variables"]
    CLI --> Queue["src/core/queue.c<br/>Ready queue"]
    CLI --> Worker["src/core/worker.c<br/>Worker pool"]
    CLI --> Metrics["src/metrics/metrics.c<br/>Final metrics"]

    Worker --> Scheduler["src/core/scheduler.c<br/>Select next job"]
    Worker --> Dispatcher["src/core/dispatcher.c<br/>Assign job to worker"]
    Worker --> Logger["src/utils/logger.c<br/>Start/finish logs"]
    Worker --> Time["src/utils/time_utils.c<br/>Scaled time simulation"]

    Scheduler --> FIFO["src/policies/fifo.c"]
    Scheduler --> SJF["src/policies/sjf.c<br/>SJF policy"]
    Scheduler --> Priority["src/policies/priority.c"]
    Scheduler --> Aging["src/policies/aging.c<br/>Bonus"]

    Logger --> Logs["logs/*.txt"]
    Metrics --> Summary["results/*.csv"]
    Logs --> Charts["src/utils/plot_logs.py<br/>SVG charts"]
    Charts --> ChartOut["results/charts/*.svg"]
```

## 3. Scheduling Decision Workflow

```mermaid
flowchart TD
    A["Worker requests job"] --> B{"Ready queue empty?"}
    B -- "Yes" --> C["Wait on job_available condition variable"]
    C --> A
    B -- "No" --> D{"Selected policy"}

    D -- "FIFO" --> E["Choose smallest sequence_no"]
    D -- "SJF" --> F["Choose shortest estimated_runtime"]
    D -- "Priority" --> G["Choose smallest priority number"]
    D -- "Aging" --> H["Choose best effective priority<br/>priority - waiting_time / AGING_INTERVAL"]

    E --> I["Remove selected job from ready queue"]
    F --> I
    G --> I
    H --> I

    I --> J["Dispatcher sets status RUNNING and start_time"]
    J --> K["Worker executes job"]
```

## 4. Experiment Workflow

```mermaid
flowchart TD
    A["make"] --> B["Build scheduler binary"]
    B --> C["./scripts/run_all.sh"]
    C --> D["Run workload A/B/C with FIFO/SJF/Priority/Aging and 4 workers"]
    D --> E["Write required logs to logs/*_4.txt"]
    E --> F["Write results/comparison_4_workers.csv"]

    B --> G["./scripts/benchmark.sh"]
    G --> H["Run workload A/B/C with FIFO/SJF/Priority/Aging and 2/4/8 workers"]
    H --> I["Write extended logs to logs/*.txt"]
    I --> J["Write results/summary.csv"]

    E --> K["python3 src/utils/plot_logs.py"]
    I --> K
    K --> L["Generate SVG charts in results/charts/"]
```

## 5. Data Flow

```mermaid
flowchart LR
    Workloads["workloads/*.csv"] --> Parser["CSV parser"]
    Parser --> Jobs["job_t array"]
    Jobs --> Arrival["Arrival simulation"]
    Arrival --> ReadyQueue["Ready queue"]
    ReadyQueue --> Scheduler["Scheduler policy"]
    Scheduler --> Dispatcher["Dispatcher"]
    Dispatcher --> Workers["Worker threads"]
    Workers --> Logs["logs/*.txt"]
    Workers --> Metrics["metrics calculation"]
    Metrics --> Results["results/*.csv"]
    Logs --> Plotter["plot_logs.py"]
    Plotter --> Charts["results/charts/*.svg"]
```
