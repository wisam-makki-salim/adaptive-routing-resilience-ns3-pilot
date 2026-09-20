#!/usr/bin/env python3
import csv
import statistics
import sys

source, summary_path, paired_path = sys.argv[1:4]
with open(source, newline="", encoding="utf-8") as handle:
    rows = list(csv.DictReader(handle))

numeric = [
    "pdr_percent", "min_flow_pdr_percent", "delay_p50_ms", "delay_p95_ms",
    "goodput_kbps", "normalized_overhead", "recovery_seconds",
    "next_hop_changes", "route_unavailable_samples", "signal_transitions",
    "failure_transitions", "recoveries",
]

def stats(values):
    values = sorted(values)
    q = statistics.quantiles(values, n=4, method="inclusive")
    return [len(values), statistics.median(values), q[0], q[2], min(values), max(values)]

with open(summary_path, "w", newline="", encoding="utf-8") as handle:
    writer = csv.writer(handle)
    writer.writerow(["scenario", "mechanism", "metric", "n", "median", "q1", "q3", "min", "max"])
    for scenario in "ABC":
        for mechanism in ("baseline", "adaptive"):
            selected = [r for r in rows if r["scenario"] == scenario and r["mechanism"] == mechanism]
            for metric in numeric:
                values = [float(r[metric]) for r in selected]
                writer.writerow([scenario, mechanism, metric, *stats(values)])

index = {(r["scenario"], r["mechanism"], int(r["run"])): r for r in rows}
with open(paired_path, "w", newline="", encoding="utf-8") as handle:
    writer = csv.writer(handle)
    writer.writerow(["scenario", "run", "metric", "adaptive_minus_baseline"])
    for scenario in "ABC":
        for run in range(101, 121):
            baseline = index[(scenario, "baseline", run)]
            adaptive = index[(scenario, "adaptive", run)]
            for metric in numeric:
                writer.writerow([scenario, run, metric, float(adaptive[metric]) - float(baseline[metric])])
