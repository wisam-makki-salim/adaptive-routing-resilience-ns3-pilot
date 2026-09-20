#!/usr/bin/env python3
import csv
import statistics
import sys

source, destination = sys.argv[1:3]
with open(source, newline="", encoding="utf-8") as handle:
    rows = list(csv.DictReader(handle))

metrics = [
    "pdr_percent",
    "min_flow_pdr_percent",
    "mean_delay_ms",
    "delay_p50_ms",
    "delay_p95_ms",
    "goodput_kbps",
    "olsr_control_packets",
    "olsr_control_bytes",
    "normalized_overhead_bytes_per_delivered_payload_byte",
    "next_hop_changes",
    "route_unavailable_samples",
]

with open(destination, "w", newline="", encoding="utf-8") as handle:
    writer = csv.writer(handle)
    writer.writerow(["metric", "n", "median", "q1", "q3", "min", "max"])
    for metric in metrics:
        values = sorted(float(row[metric]) for row in rows)
        quartiles = statistics.quantiles(values, n=4, method="inclusive")
        writer.writerow(
            [metric, len(values), statistics.median(values), quartiles[0], quartiles[2], min(values), max(values)]
        )
