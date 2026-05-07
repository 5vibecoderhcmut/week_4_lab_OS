#!/usr/bin/env python3
"""
Generate SVG charts from scheduler log files.

Usage:
    python3 src/utils/plot_logs.py --logs logs --out results/charts

The script scans files named like:
    logs/a_fifo_4.txt
    logs/b_priority_8.txt

It parses the summary block at the end of each log and creates SVG charts
without external Python dependencies.
"""

from __future__ import annotations

import argparse
import csv
import html
import re
from dataclasses import dataclass
from pathlib import Path


POLICY_ORDER = ["fifo", "sjf", "priority", "aging"]
POLICY_LABELS = {
    "fifo": "FIFO",
    "sjf": "SJF",
    "priority": "Priority",
    "aging": "Aging",
}
POLICY_COLORS = {
    "fifo": "#2563eb",
    "sjf": "#16a34a",
    "priority": "#d97706",
    "aging": "#7c3aed",
}
WORKLOAD_LABELS = {
    "a": "Workload A",
    "b": "Workload B",
    "c": "Workload C",
}
DATASET_COLORS = [
    "#2563eb",
    "#16a34a",
    "#d97706",
    "#7c3aed",
    "#0f766e",
    "#dc2626",
    "#0891b2",
    "#4b5563",
]


@dataclass(frozen=True)
class LogMetrics:
    workload: str
    policy: str
    workers: int
    total_jobs: int
    total_simulation_time: float
    avg_waiting: float
    avg_turnaround: float
    throughput: float
    utilization_percent: float
    starvation_risk_jobs: int
    log_file: Path


@dataclass(frozen=True)
class WorkloadStats:
    workload: str
    job_count: int
    total_runtime: int
    avg_runtime: float
    min_runtime: int
    max_runtime: int
    first_arrival: int
    last_arrival: int
    arrival_span: int
    avg_priority: float
    priority_counts: dict[int, int]
    job_type_counts: dict[str, int]
    source_file: Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate SVG charts from scheduler logs.")
    parser.add_argument("--logs", default="logs", help="Directory containing scheduler .txt logs")
    parser.add_argument("--workloads", default="workloads", help="Directory containing workload CSV files")
    parser.add_argument("--out", default="results/charts", help="Output directory for SVG charts")
    parser.add_argument(
        "--dataset-summary",
        default=None,
        help="Output CSV for workload dataset statistics. Defaults to <chart output parent>/dataset_stats.csv",
    )
    parser.add_argument("--workers", type=int, default=4, help="Worker count for policy comparison charts")
    return parser.parse_args()


def parse_summary_value(lines: list[str], key: str) -> str | None:
    prefix = f"{key}:"
    for line in lines:
        if line.startswith(prefix):
            return line.split(":", 1)[1].strip()
    return None


def parse_float_from_value(value: str | None) -> float:
    if value is None:
        raise ValueError("missing value")
    match = re.search(r"-?\d+(?:\.\d+)?", value)
    if match is None:
        raise ValueError(f"cannot parse numeric value from {value!r}")
    return float(match.group(0))


def parse_int_from_value(value: str | None) -> int:
    return int(parse_float_from_value(value))


def normalize_policy(filename_policy: str, summary_policy: str | None) -> str:
    raw = filename_policy.lower()
    if raw in {"fcfs"}:
        return "fifo"
    if raw in {"prio"}:
        return "priority"
    if raw in {"aging-priority"}:
        return "aging"
    if raw in POLICY_LABELS:
        return raw

    if summary_policy:
        summary = summary_policy.lower()
        if "fifo" in summary:
            return "fifo"
        if "sjf" in summary:
            return "sjf"
        if "priority" in summary and "aging" not in summary:
            return "priority"
        if "aging" in summary:
            return "aging"

    return raw


def parse_log(path: Path) -> LogMetrics | None:
    match = re.match(r"([abc])_([a-zA-Z-]+)_(\d+)\.txt$", path.name)
    if match is None:
        return None

    workload = match.group(1).lower()
    filename_policy = match.group(2).lower()
    workers = int(match.group(3))
    lines = path.read_text(encoding="utf-8").splitlines()
    policy = normalize_policy(filename_policy, parse_summary_value(lines, "Policy"))

    try:
        return LogMetrics(
            workload=workload,
            policy=policy,
            workers=workers,
            total_jobs=parse_int_from_value(parse_summary_value(lines, "Total jobs")),
            total_simulation_time=parse_float_from_value(parse_summary_value(lines, "Total simulation time")),
            avg_waiting=parse_float_from_value(parse_summary_value(lines, "Average waiting time")),
            avg_turnaround=parse_float_from_value(parse_summary_value(lines, "Average turnaround time")),
            throughput=parse_float_from_value(parse_summary_value(lines, "Throughput")),
            utilization_percent=parse_float_from_value(parse_summary_value(lines, "Worker utilization")),
            starvation_risk_jobs=parse_int_from_value(parse_summary_value(lines, "Starvation-risk jobs")),
            log_file=path,
        )
    except ValueError as exc:
        raise ValueError(f"Cannot parse summary in {path}: {exc}") from exc


def load_metrics(log_dir: Path) -> list[LogMetrics]:
    rows: list[LogMetrics] = []
    for path in sorted(log_dir.glob("*.txt")):
        parsed = parse_log(path)
        if parsed is not None:
            rows.append(parsed)
    return rows


def workload_name_from_path(path: Path) -> str | None:
    match = re.match(r"workload_([abc])\.csv$", path.name)
    if match is None:
        return None
    return match.group(1).lower()


def load_workload_stats(workload_dir: Path) -> list[WorkloadStats]:
    stats: list[WorkloadStats] = []

    for path in sorted(workload_dir.glob("workload_*.csv")):
        workload = workload_name_from_path(path)
        if workload is None:
            continue

        runtimes: list[int] = []
        arrivals: list[int] = []
        priorities: list[int] = []
        priority_counts: dict[int, int] = {}
        job_type_counts: dict[str, int] = {}

        with path.open("r", encoding="utf-8", newline="") as file:
            reader = csv.DictReader(file)
            for row in reader:
                runtime = int(row["estimated_runtime"])
                arrival = int(row["arrival_time"])
                priority = int(row["priority"])
                job_type = row["job_type"].strip()

                runtimes.append(runtime)
                arrivals.append(arrival)
                priorities.append(priority)
                priority_counts[priority] = priority_counts.get(priority, 0) + 1
                job_type_counts[job_type] = job_type_counts.get(job_type, 0) + 1

        if not runtimes:
            continue

        stats.append(
            WorkloadStats(
                workload=workload,
                job_count=len(runtimes),
                total_runtime=sum(runtimes),
                avg_runtime=sum(runtimes) / len(runtimes),
                min_runtime=min(runtimes),
                max_runtime=max(runtimes),
                first_arrival=min(arrivals),
                last_arrival=max(arrivals),
                arrival_span=max(arrivals) - min(arrivals),
                avg_priority=sum(priorities) / len(priorities),
                priority_counts=priority_counts,
                job_type_counts=job_type_counts,
                source_file=path,
            )
        )

    return stats


def write_dataset_stats_csv(stats: list[WorkloadStats], out_file: Path) -> None:
    priorities = sorted({priority for row in stats for priority in row.priority_counts})
    job_types = sorted({job_type for row in stats for job_type in row.job_type_counts})
    fields = [
        "workload",
        "source_file",
        "job_count",
        "total_runtime",
        "avg_runtime",
        "min_runtime",
        "max_runtime",
        "first_arrival",
        "last_arrival",
        "arrival_span",
        "avg_priority",
    ]
    fields += [f"priority_{priority}" for priority in priorities]
    fields += [f"type_{job_type}" for job_type in job_types]

    out_file.parent.mkdir(parents=True, exist_ok=True)
    with out_file.open("w", encoding="utf-8", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=fields)
        writer.writeheader()
        for row in sorted(stats, key=lambda item: item.workload):
            data: dict[str, str | int] = {
                "workload": row.workload,
                "source_file": str(row.source_file),
                "job_count": row.job_count,
                "total_runtime": row.total_runtime,
                "avg_runtime": f"{row.avg_runtime:.2f}",
                "min_runtime": row.min_runtime,
                "max_runtime": row.max_runtime,
                "first_arrival": row.first_arrival,
                "last_arrival": row.last_arrival,
                "arrival_span": row.arrival_span,
                "avg_priority": f"{row.avg_priority:.2f}",
            }
            for priority in priorities:
                data[f"priority_{priority}"] = row.priority_counts.get(priority, 0)
            for job_type in job_types:
                data[f"type_{job_type}"] = row.job_type_counts.get(job_type, 0)
            writer.writerow(data)


def svg_text(x: float, y: float, text: str, size: int = 13, anchor: str = "middle", weight: str = "400") -> str:
    return (
        f'<text x="{x:.1f}" y="{y:.1f}" font-family="Arial, sans-serif" '
        f'font-size="{size}" font-weight="{weight}" text-anchor="{anchor}" '
        f'fill="#111827">{html.escape(text)}</text>'
    )


def nice_max(value: float) -> float:
    if value <= 0:
        return 1.0

    magnitude = 1.0
    while value >= magnitude * 10.0:
        magnitude *= 10.0
    while value < magnitude:
        magnitude /= 10.0

    normalized = value / magnitude
    if normalized <= 1:
        nice = 1
    elif normalized <= 2:
        nice = 2
    elif normalized <= 5:
        nice = 5
    else:
        nice = 10
    return nice * magnitude


def grouped_bar_chart(
    rows: list[LogMetrics],
    metric_name: str,
    title: str,
    y_label: str,
    out_file: Path,
    value_suffix: str = "",
) -> None:
    width = 980
    height = 560
    margin_left = 82
    margin_right = 32
    margin_top = 78
    margin_bottom = 86
    plot_width = width - margin_left - margin_right
    plot_height = height - margin_top - margin_bottom

    workloads = sorted({row.workload for row in rows})
    policies = [policy for policy in POLICY_ORDER if any(row.policy == policy for row in rows)]
    lookup = {(row.workload, row.policy): row for row in rows}
    values = [float(getattr(row, metric_name)) for row in rows]
    max_value = nice_max(max(values) * 1.12 if values else 1)

    group_width = plot_width / max(len(workloads), 1)
    inner_width = group_width * 0.72
    bar_gap = 8
    bar_width = (inner_width - bar_gap * (len(policies) - 1)) / max(len(policies), 1)

    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#ffffff"/>',
        svg_text(width / 2, 34, title, size=22, weight="700"),
        svg_text(width / 2, 58, "Generated from scheduler log summaries", size=12),
    ]

    for i in range(6):
        value = max_value * i / 5
        y = margin_top + plot_height - (value / max_value) * plot_height
        parts.append(f'<line x1="{margin_left}" y1="{y:.1f}" x2="{width - margin_right}" y2="{y:.1f}" stroke="#e5e7eb"/>')
        parts.append(svg_text(margin_left - 12, y + 4, f"{value:.1f}", size=11, anchor="end"))

    parts.append(f'<line x1="{margin_left}" y1="{margin_top}" x2="{margin_left}" y2="{margin_top + plot_height}" stroke="#374151"/>')
    parts.append(f'<line x1="{margin_left}" y1="{margin_top + plot_height}" x2="{width - margin_right}" y2="{margin_top + plot_height}" stroke="#374151"/>')
    parts.append(svg_text(24, margin_top + plot_height / 2, y_label, size=12, anchor="middle"))

    for workload_index, workload in enumerate(workloads):
        group_x = margin_left + workload_index * group_width
        start_x = group_x + (group_width - inner_width) / 2
        label_x = group_x + group_width / 2
        parts.append(svg_text(label_x, height - 38, WORKLOAD_LABELS.get(workload, workload.upper()), size=13, weight="700"))

        for policy_index, policy in enumerate(policies):
            row = lookup.get((workload, policy))
            if row is None:
                continue

            value = float(getattr(row, metric_name))
            bar_height = (value / max_value) * plot_height if max_value else 0
            x = start_x + policy_index * (bar_width + bar_gap)
            y = margin_top + plot_height - bar_height
            color = POLICY_COLORS.get(policy, "#4b5563")
            parts.append(
                f'<rect x="{x:.1f}" y="{y:.1f}" width="{bar_width:.1f}" height="{bar_height:.1f}" '
                f'fill="{color}" rx="3"/>'
            )
            parts.append(svg_text(x + bar_width / 2, y - 6, f"{value:g}{value_suffix}", size=10))

    legend_x = margin_left
    legend_y = height - 18
    for policy in policies:
        color = POLICY_COLORS.get(policy, "#4b5563")
        label = POLICY_LABELS.get(policy, policy)
        parts.append(f'<rect x="{legend_x}" y="{legend_y - 10}" width="12" height="12" fill="{color}" rx="2"/>')
        parts.append(svg_text(legend_x + 18, legend_y, label, size=12, anchor="start"))
        legend_x += 96

    parts.append("</svg>")
    out_file.write_text("\n".join(parts) + "\n", encoding="utf-8")


def generic_grouped_bar_chart(
    categories: list[str],
    series: list[tuple[str, str, list[float]]],
    title: str,
    subtitle: str,
    y_label: str,
    out_file: Path,
    value_suffix: str = "",
) -> None:
    width = 980
    height = 560
    margin_left = 82
    margin_right = 32
    margin_top = 78
    margin_bottom = 92
    plot_width = width - margin_left - margin_right
    plot_height = height - margin_top - margin_bottom
    values = [value for _, _, series_values in series for value in series_values]
    max_value = nice_max(max(values) * 1.12 if values else 1)
    group_width = plot_width / max(len(categories), 1)
    inner_width = group_width * 0.74
    bar_gap = 8
    bar_width = (inner_width - bar_gap * (len(series) - 1)) / max(len(series), 1)

    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#ffffff"/>',
        svg_text(width / 2, 34, title, size=22, weight="700"),
        svg_text(width / 2, 58, subtitle, size=12),
    ]

    for i in range(6):
        value = max_value * i / 5
        y = margin_top + plot_height - (value / max_value) * plot_height
        parts.append(f'<line x1="{margin_left}" y1="{y:.1f}" x2="{width - margin_right}" y2="{y:.1f}" stroke="#e5e7eb"/>')
        parts.append(svg_text(margin_left - 12, y + 4, f"{value:.1f}", size=11, anchor="end"))

    parts.append(f'<line x1="{margin_left}" y1="{margin_top}" x2="{margin_left}" y2="{margin_top + plot_height}" stroke="#374151"/>')
    parts.append(f'<line x1="{margin_left}" y1="{margin_top + plot_height}" x2="{width - margin_right}" y2="{margin_top + plot_height}" stroke="#374151"/>')
    parts.append(svg_text(24, margin_top + plot_height / 2, y_label, size=12, anchor="middle"))

    for category_index, category in enumerate(categories):
        group_x = margin_left + category_index * group_width
        start_x = group_x + (group_width - inner_width) / 2
        label_x = group_x + group_width / 2
        parts.append(svg_text(label_x, height - 42, category, size=13, weight="700"))

        for series_index, (label, color, series_values) in enumerate(series):
            value = series_values[category_index]
            bar_height = (value / max_value) * plot_height if max_value else 0
            x = start_x + series_index * (bar_width + bar_gap)
            y = margin_top + plot_height - bar_height
            parts.append(
                f'<rect x="{x:.1f}" y="{y:.1f}" width="{bar_width:.1f}" height="{bar_height:.1f}" '
                f'fill="{color}" rx="3"/>'
            )
            parts.append(svg_text(x + bar_width / 2, y - 6, f"{value:g}{value_suffix}", size=10))

    legend_x = margin_left
    legend_y = height - 18
    for label, color, _ in series:
        parts.append(f'<rect x="{legend_x}" y="{legend_y - 10}" width="12" height="12" fill="{color}" rx="2"/>')
        parts.append(svg_text(legend_x + 18, legend_y, label, size=12, anchor="start"))
        legend_x += max(94, len(label) * 8 + 34)

    parts.append("</svg>")
    out_file.write_text("\n".join(parts) + "\n", encoding="utf-8")


def generate_dataset_charts(stats: list[WorkloadStats], out_dir: Path) -> list[Path]:
    ordered = sorted(stats, key=lambda item: item.workload)
    categories = [WORKLOAD_LABELS.get(row.workload, row.workload.upper()) for row in ordered]
    chart_files: list[Path] = []

    runtime_chart = out_dir / "dataset_runtime_profile.svg"
    generic_grouped_bar_chart(
        categories,
        [
            ("Min runtime", DATASET_COLORS[0], [float(row.min_runtime) for row in ordered]),
            ("Avg runtime", DATASET_COLORS[1], [row.avg_runtime for row in ordered]),
            ("Max runtime", DATASET_COLORS[2], [float(row.max_runtime) for row in ordered]),
        ],
        "Dataset Runtime Profile",
        "Generated from workloads/workload_a.csv, workload_b.csv, workload_c.csv",
        "Runtime units",
        runtime_chart,
    )
    chart_files.append(runtime_chart)

    size_chart = out_dir / "dataset_size_arrival.svg"
    generic_grouped_bar_chart(
        categories,
        [
            ("Job count", DATASET_COLORS[0], [float(row.job_count) for row in ordered]),
            ("Arrival span", DATASET_COLORS[3], [float(row.arrival_span) for row in ordered]),
        ],
        "Dataset Size and Arrival Span",
        "Job count and simulated arrival-time range",
        "Count / time units",
        size_chart,
    )
    chart_files.append(size_chart)

    total_runtime_chart = out_dir / "dataset_total_runtime.svg"
    generic_grouped_bar_chart(
        categories,
        [
            ("Total runtime", DATASET_COLORS[4], [float(row.total_runtime) for row in ordered]),
        ],
        "Dataset Total Runtime",
        "Sum of estimated_runtime for each workload",
        "Runtime units",
        total_runtime_chart,
    )
    chart_files.append(total_runtime_chart)

    priorities = sorted({priority for row in ordered for priority in row.priority_counts})
    priority_chart = out_dir / "dataset_priority_distribution.svg"
    generic_grouped_bar_chart(
        categories,
        [
            (f"Priority {priority}", DATASET_COLORS[index % len(DATASET_COLORS)], [float(row.priority_counts.get(priority, 0)) for row in ordered])
            for index, priority in enumerate(priorities)
        ],
        "Dataset Priority Distribution",
        "Smaller priority number means higher priority",
        "Jobs",
        priority_chart,
    )
    chart_files.append(priority_chart)

    job_types = sorted({job_type for row in ordered for job_type in row.job_type_counts})
    job_type_chart = out_dir / "dataset_job_type_distribution.svg"
    generic_grouped_bar_chart(
        categories,
        [
            (
                job_type.replace("_", " ").title(),
                DATASET_COLORS[index % len(DATASET_COLORS)],
                [float(row.job_type_counts.get(job_type, 0)) for row in ordered],
            )
            for index, job_type in enumerate(job_types)
        ],
        "Dataset Job Type Distribution",
        "Count of image-processing job types in each workload",
        "Jobs",
        job_type_chart,
    )
    chart_files.append(job_type_chart)

    return chart_files


def scaling_line_chart(rows: list[LogMetrics], workload: str, out_file: Path) -> None:
    width = 980
    height = 560
    margin_left = 82
    margin_right = 32
    margin_top = 78
    margin_bottom = 82
    plot_width = width - margin_left - margin_right
    plot_height = height - margin_top - margin_bottom

    workload_rows = [row for row in rows if row.workload == workload]
    workers = sorted({row.workers for row in workload_rows})
    policies = [policy for policy in POLICY_ORDER if any(row.policy == policy for row in workload_rows)]
    values = [row.avg_waiting for row in workload_rows]
    max_value = nice_max(max(values) * 1.12 if values else 1)

    def x_for(worker_count: int) -> float:
        if len(workers) == 1:
            return margin_left + plot_width / 2
        index = workers.index(worker_count)
        return margin_left + (index / (len(workers) - 1)) * plot_width

    def y_for(value: float) -> float:
        return margin_top + plot_height - (value / max_value) * plot_height

    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#ffffff"/>',
        svg_text(width / 2, 34, f"Worker Scaling - {WORKLOAD_LABELS.get(workload, workload.upper())}", size=22, weight="700"),
        svg_text(width / 2, 58, "Average waiting time by worker count", size=12),
    ]

    for i in range(6):
        value = max_value * i / 5
        y = y_for(value)
        parts.append(f'<line x1="{margin_left}" y1="{y:.1f}" x2="{width - margin_right}" y2="{y:.1f}" stroke="#e5e7eb"/>')
        parts.append(svg_text(margin_left - 12, y + 4, f"{value:.1f}", size=11, anchor="end"))

    parts.append(f'<line x1="{margin_left}" y1="{margin_top}" x2="{margin_left}" y2="{margin_top + plot_height}" stroke="#374151"/>')
    parts.append(f'<line x1="{margin_left}" y1="{margin_top + plot_height}" x2="{width - margin_right}" y2="{margin_top + plot_height}" stroke="#374151"/>')
    parts.append(svg_text(width / 2, height - 22, "Workers", size=13, weight="700"))
    parts.append(svg_text(24, margin_top + plot_height / 2, "Avg waiting time", size=12))

    for worker_count in workers:
        x = x_for(worker_count)
        parts.append(f'<line x1="{x:.1f}" y1="{margin_top + plot_height}" x2="{x:.1f}" y2="{margin_top + plot_height + 6}" stroke="#374151"/>')
        parts.append(svg_text(x, margin_top + plot_height + 26, str(worker_count), size=12))

    for policy in policies:
        policy_rows = sorted((row for row in workload_rows if row.policy == policy), key=lambda row: row.workers)
        if not policy_rows:
            continue
        color = POLICY_COLORS.get(policy, "#4b5563")
        points = [(x_for(row.workers), y_for(row.avg_waiting), row.avg_waiting) for row in policy_rows]
        point_string = " ".join(f"{x:.1f},{y:.1f}" for x, y, _ in points)
        parts.append(f'<polyline points="{point_string}" fill="none" stroke="{color}" stroke-width="3"/>')
        for x, y, value in points:
            parts.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="5" fill="{color}"/>')
            parts.append(svg_text(x, y - 10, f"{value:g}", size=10))

    legend_x = margin_left
    legend_y = height - 48
    for policy in policies:
        color = POLICY_COLORS.get(policy, "#4b5563")
        label = POLICY_LABELS.get(policy, policy)
        parts.append(f'<rect x="{legend_x}" y="{legend_y - 10}" width="12" height="12" fill="{color}" rx="2"/>')
        parts.append(svg_text(legend_x + 18, legend_y, label, size=12, anchor="start"))
        legend_x += 96

    parts.append("</svg>")
    out_file.write_text("\n".join(parts) + "\n", encoding="utf-8")


def write_index(out_dir: Path, chart_files: list[Path]) -> None:
    links = "\n".join(
        f'    <li><a href="{html.escape(path.name)}">{html.escape(path.stem.replace("_", " ").title())}</a></li>'
        for path in chart_files
    )
    html_text = f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>Scheduler Charts</title>
  <style>
    body {{ font-family: Arial, sans-serif; margin: 32px; color: #111827; }}
    a {{ color: #2563eb; }}
    li {{ margin: 8px 0; }}
  </style>
</head>
<body>
  <h1>Scheduler Charts</h1>
  <p>Generated from log summaries.</p>
  <ul>
{links}
  </ul>
</body>
</html>
"""
    (out_dir / "index.html").write_text(html_text, encoding="utf-8")


def main() -> int:
    args = parse_args()
    log_dir = Path(args.logs)
    workload_dir = Path(args.workloads)
    out_dir = Path(args.out)

    if not log_dir.exists():
        raise SystemExit(f"Log directory does not exist: {log_dir}")

    rows = load_metrics(log_dir)
    if not rows:
        raise SystemExit(f"No parseable scheduler logs found in {log_dir}")

    out_dir.mkdir(parents=True, exist_ok=True)

    comparison_rows = [
        row
        for row in rows
        if row.workers == args.workers and row.policy in set(POLICY_ORDER)
    ]
    if not comparison_rows:
        raise SystemExit(f"No logs found for workers={args.workers}")

    chart_files = [
        out_dir / "avg_waiting_4_workers.svg",
        out_dir / "avg_turnaround_4_workers.svg",
        out_dir / "throughput_4_workers.svg",
        out_dir / "utilization_4_workers.svg",
        out_dir / "starvation_4_workers.svg",
    ]

    grouped_bar_chart(
        comparison_rows,
        "avg_waiting",
        f"Average Waiting Time ({args.workers} Workers)",
        "Time units",
        chart_files[0],
    )
    grouped_bar_chart(
        comparison_rows,
        "avg_turnaround",
        f"Average Turnaround Time ({args.workers} Workers)",
        "Time units",
        chart_files[1],
    )
    grouped_bar_chart(
        comparison_rows,
        "throughput",
        f"Throughput ({args.workers} Workers)",
        "Jobs / time unit",
        chart_files[2],
    )
    grouped_bar_chart(
        comparison_rows,
        "utilization_percent",
        f"Worker Utilization ({args.workers} Workers)",
        "Percent",
        chart_files[3],
        value_suffix="%",
    )
    grouped_bar_chart(
        comparison_rows,
        "starvation_risk_jobs",
        f"Starvation-Risk Jobs ({args.workers} Workers)",
        "Jobs",
        chart_files[4],
    )

    for workload in sorted({row.workload for row in rows}):
        chart_file = out_dir / f"worker_scaling_{workload}.svg"
        scaling_line_chart(rows, workload, chart_file)
        chart_files.append(chart_file)

    workload_stats = load_workload_stats(workload_dir) if workload_dir.exists() else []
    dataset_stats_file = Path(args.dataset_summary) if args.dataset_summary else out_dir.parent / "dataset_stats.csv"
    if workload_stats:
        write_dataset_stats_csv(workload_stats, dataset_stats_file)
        chart_files.extend(generate_dataset_charts(workload_stats, out_dir))

    write_index(out_dir, chart_files)

    print(f"Parsed {len(rows)} log files")
    if workload_stats:
        print(f"Parsed {len(workload_stats)} workload datasets")
        print(f"Dataset stats written to {dataset_stats_file}")
    print(f"Charts written to {out_dir}")
    for chart_file in chart_files:
        print(f"- {chart_file}")
    print(f"- {out_dir / 'index.html'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
