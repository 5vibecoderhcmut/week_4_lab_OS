#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p logs results
make

summary="results/summary.csv"
printf "workload,policy,workers,total_jobs,total_simulation_time,avg_waiting,avg_turnaround,throughput,utilization_percent,starvation_risk_jobs,log_file\n" > "$summary"

extract_value() {
  local key="$1"
  local file="$2"
  awk -F': ' -v key="$key" '$1 == key {print $2}' "$file" | tail -n 1
}

for workload in a b c; do
  for policy in fifo sjf priority aging; do
    for workers in 2 4 8; do
      input="workloads/workload_${workload}.csv"
      output="logs/${workload}_${policy}_${workers}.txt"
      echo "Running workload=${workload} policy=${policy} workers=${workers}"
      ./scheduler "$input" "$policy" "$workers" > "$output"

      total_jobs="$(extract_value "Total jobs" "$output")"
      total_time="$(extract_value "Total simulation time" "$output")"
      avg_waiting="$(extract_value "Average waiting time" "$output")"
      avg_turnaround="$(extract_value "Average turnaround time" "$output")"
      throughput="$(extract_value "Throughput" "$output" | awk '{print $1}')"
      utilization="$(extract_value "Worker utilization" "$output" | tr -d '%')"
      starvation="$(extract_value "Starvation-risk jobs" "$output")"
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n" \
        "$workload" "$policy" "$workers" "$total_jobs" "$total_time" "$avg_waiting" \
        "$avg_turnaround" "$throughput" "$utilization" "$starvation" "$output" >> "$summary"
    done
  done
done

echo "Benchmark table written to $summary"
