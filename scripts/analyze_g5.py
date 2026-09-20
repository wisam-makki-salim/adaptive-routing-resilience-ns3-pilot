#!/usr/bin/env python3
"""Deterministic paired uncertainty analysis and G5 figure generation."""

import csv
import pathlib
import sys

import matplotlib.pyplot as plt
import numpy as np


raw_path = pathlib.Path(sys.argv[1])
results_dir = pathlib.Path(sys.argv[2])
figures_dir = pathlib.Path(sys.argv[3])
trace_dir = pathlib.Path(sys.argv[4])
results_dir.mkdir(parents=True, exist_ok=True)
figures_dir.mkdir(parents=True, exist_ok=True)

with raw_path.open(newline="", encoding="utf-8") as handle:
    rows = list(csv.DictReader(handle))

metrics = [
    "pdr_percent",
    "min_flow_pdr_percent",
    "delay_p95_ms",
    "goodput_kbps",
    "normalized_overhead",
    "recovery_seconds",
    "next_hop_changes",
    "route_unavailable_samples",
]
index = {(r["scenario"], r["mechanism"], int(r["run"])): r for r in rows}
runs = sorted({int(r["run"]) for r in rows})
rng = np.random.default_rng(20260920)
bootstrap_samples = 50_000


def paired_values(scenario, metric):
    return np.array(
        [
            float(index[(scenario, "adaptive", run)][metric])
            - float(index[(scenario, "baseline", run)][metric])
            for run in runs
        ],
        dtype=float,
    )


def interval(values, statistic):
    sampled = values[rng.integers(0, len(values), size=(bootstrap_samples, len(values)))]
    estimates = statistic(sampled, axis=1)
    return np.percentile(estimates, [2.5, 97.5])


with (results_dir / "g5_paired_uncertainty.csv").open(
    "w", newline="", encoding="utf-8"
) as handle:
    writer = csv.writer(handle)
    writer.writerow(
        [
            "scenario",
            "metric",
            "n_pairs",
            "mean_difference",
            "mean_ci95_low",
            "mean_ci95_high",
            "median_difference",
            "median_ci95_low",
            "median_ci95_high",
            "q1",
            "q3",
            "min",
            "max",
            "bootstrap_samples",
            "bootstrap_seed",
        ]
    )
    for scenario in "ABC":
        for metric in metrics:
            values = paired_values(scenario, metric)
            mean_ci = interval(values, np.mean)
            median_ci = interval(values, np.median)
            writer.writerow(
                [
                    scenario,
                    metric,
                    len(values),
                    *[f"{x:.9f}" for x in [np.mean(values), *mean_ci]],
                    *[f"{x:.9f}" for x in [np.median(values), *median_ci]],
                    *[f"{x:.9f}" for x in np.percentile(values, [25, 75])],
                    f"{np.min(values):.9f}",
                    f"{np.max(values):.9f}",
                    bootstrap_samples,
                    20260920,
                ]
            )

with (results_dir / "g5_direction_counts.csv").open(
    "w", newline="", encoding="utf-8"
) as handle:
    writer = csv.writer(handle)
    writer.writerow(["scenario", "metric", "adaptive_lower", "equal", "adaptive_higher"])
    for scenario in "ABC":
        for metric in metrics:
            values = paired_values(scenario, metric)
            writer.writerow(
                [scenario, metric, int(np.sum(values < 0)), int(np.sum(values == 0)), int(np.sum(values > 0))]
            )

with (results_dir / "g5_run102_comparison.csv").open(
    "w", newline="", encoding="utf-8"
) as handle:
    writer = csv.writer(handle)
    writer.writerow(["metric", "baseline", "adaptive", "adaptive_minus_baseline"])
    for metric in metrics:
        baseline = float(index[("C", "baseline", 102)][metric])
        adaptive = float(index[("C", "adaptive", 102)][metric])
        writer.writerow([metric, baseline, adaptive, adaptive - baseline])

plt.rcParams.update(
    {
        "font.size": 9,
        "axes.titlesize": 10,
        "axes.labelsize": 9,
        "legend.fontsize": 8,
        "figure.dpi": 150,
        "savefig.dpi": 300,
    }
)

# Figure 1: paired PDR effects retain seed identity and expose tail behavior.
fig, axes = plt.subplots(1, 2, figsize=(7.2, 3.0), sharey=True)
for axis, scenario in zip(axes, "BC"):
    values = paired_values(scenario, "pdr_percent")
    colors = ["#c23b22" if scenario == "C" and run == 102 else "#2463a6" for run in runs]
    axis.axhline(0, color="black", linewidth=0.8)
    axis.scatter(runs, values, c=colors, s=24, zorder=3)
    axis.plot(runs, values, color="#9bb7d3", linewidth=0.7, zorder=2)
    axis.set_title(f"Scenario {scenario}")
    axis.set_xlabel("Paired run (seed index)")
    axis.grid(axis="y", alpha=0.25)
axes[0].set_ylabel("Adaptive − baseline PDR (percentage points)")
fig.suptitle("Paired PDR differences across final-evaluation seeds", y=1.02)
fig.tight_layout()
fig.savefig(figures_dir / "g5_paired_pdr_differences.png", bbox_inches="tight")
fig.savefig(figures_dir / "g5_paired_pdr_differences.pdf", bbox_inches="tight")
plt.close(fig)

# Figure 2: performance/overhead trade-off for every paired run.
fig, axis = plt.subplots(figsize=(5.5, 3.7))
for scenario, marker, color in [("B", "o", "#2463a6"), ("C", "s", "#d06b32")]:
    pdr = paired_values(scenario, "pdr_percent")
    overhead = paired_values(scenario, "normalized_overhead")
    axis.scatter(overhead, pdr, marker=marker, color=color, alpha=0.8, label=f"Scenario {scenario}")
axis.axhline(0, color="black", linewidth=0.8)
axis.axvline(0, color="black", linewidth=0.8)
axis.set_xlabel("Adaptive − baseline normalized overhead")
axis.set_ylabel("Adaptive − baseline PDR (percentage points)")
axis.set_title("Paired resilience–overhead trade-off")
axis.grid(alpha=0.25)
axis.legend(frameon=False)
fig.tight_layout()
fig.savefig(figures_dir / "g5_overhead_pdr_tradeoff.png", bbox_inches="tight")
fig.savefig(figures_dir / "g5_overhead_pdr_tradeoff.pdf", bbox_inches="tight")
plt.close(fig)


def read_rx(mechanism):
    with (trace_dir / f"C_{mechanism}_102_rx.csv").open(newline="", encoding="utf-8") as handle:
        data = list(csv.DictReader(handle))
    return np.array([int(r["second"]) for r in data]), np.array([int(r["rx_packets"]) for r in data])


# Figure 3: diagnostic trace for the retained adverse-tail run.
fig, axis = plt.subplots(figsize=(7.2, 3.2))
for mechanism, color in [("baseline", "#555555"), ("adaptive", "#c23b22")]:
    seconds, received = read_rx(mechanism)
    mask = (seconds >= 45) & (seconds <= 90)
    axis.step(seconds[mask], received[mask], where="mid", color=color, label=mechanism.capitalize())
axis.axvline(60, color="black", linestyle="--", linewidth=1, label="Node failure")
axis.set_xlabel("Simulation time (s)")
axis.set_ylabel("Received application packets / s")
axis.set_title("Scenario C, run 102: adverse-tail diagnostic")
axis.grid(alpha=0.25)
axis.legend(frameon=False, ncol=3)
fig.tight_layout()
fig.savefig(figures_dir / "g5_run102_rx_timeline.png", bbox_inches="tight")
fig.savefig(figures_dir / "g5_run102_rx_timeline.pdf", bbox_inches="tight")
plt.close(fig)
