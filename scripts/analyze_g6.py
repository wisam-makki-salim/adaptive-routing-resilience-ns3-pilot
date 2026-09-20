#!/usr/bin/env python3
"""Analyze the predeclared G6 component ablations and threshold sensitivity."""

import csv
import pathlib
import sys

import matplotlib.pyplot as plt
import numpy as np


g4_path, g6_path, output_arg, figures_arg = map(pathlib.Path, sys.argv[1:5])
output_arg.mkdir(parents=True, exist_ok=True)
figures_arg.mkdir(parents=True, exist_ok=True)
with g4_path.open(newline="", encoding="utf-8") as handle:
    g4 = list(csv.DictReader(handle))
with g6_path.open(newline="", encoding="utf-8") as handle:
    g6 = list(csv.DictReader(handle))

g4_index = {(r["scenario"], r["mechanism"], int(r["run"])): r for r in g4}
g6_index = {(r["scenario"], r["variant"], int(r["run"])): r for r in g6}
runs = list(range(101, 121))
variants = {
    "C": ["full_adaptive", "detection_only", "hello_only", "tc_only", "failure20", "failure80"],
    "B": ["signal78", "full_adaptive", "signal84"],
}
metrics = [
    "pdr_percent",
    "min_flow_pdr_percent",
    "delay_p95_ms",
    "goodput_kbps",
    "normalized_overhead",
    "recovery_seconds",
    "next_hop_changes",
]
rng = np.random.default_rng(20260920)


def variant_row(scenario, variant, run):
    if variant == "full_adaptive":
        return g4_index[(scenario, "adaptive", run)]
    return g6_index[(scenario, variant, run)]


def differences(scenario, variant, metric):
    return np.array(
        [
            float(variant_row(scenario, variant, run)[metric])
            - float(g4_index[(scenario, "baseline", run)][metric])
            for run in runs
        ]
    )


def bootstrap_ci(values, statistic):
    sample = values[rng.integers(0, len(values), size=(50_000, len(values)))]
    return np.percentile(statistic(sample, axis=1), [2.5, 97.5])


with (output_arg / "g6_paired_uncertainty.csv").open("w", newline="", encoding="utf-8") as handle:
    writer = csv.writer(handle)
    writer.writerow(
        ["scenario", "variant", "metric", "n_pairs", "mean_difference", "mean_ci95_low",
         "mean_ci95_high", "median_difference", "median_ci95_low", "median_ci95_high",
         "adaptive_lower", "equal", "adaptive_higher"]
    )
    for scenario, scenario_variants in variants.items():
        for variant in scenario_variants:
            for metric in metrics:
                values = differences(scenario, variant, metric)
                mean_ci = bootstrap_ci(values, np.mean)
                median_ci = bootstrap_ci(values, np.median)
                writer.writerow(
                    [scenario, variant, metric, len(values), f"{np.mean(values):.9f}",
                     f"{mean_ci[0]:.9f}", f"{mean_ci[1]:.9f}", f"{np.median(values):.9f}",
                     f"{median_ci[0]:.9f}", f"{median_ci[1]:.9f}", int(np.sum(values < 0)),
                     int(np.sum(values == 0)), int(np.sum(values > 0))]
                )

with (output_arg / "g6_run102_ablation.csv").open("w", newline="", encoding="utf-8") as handle:
    writer = csv.writer(handle)
    writer.writerow(["variant", "pdr_percent", "normalized_overhead", "recovery_seconds",
                     "failure_transitions", "pdr_difference_vs_baseline"])
    baseline = g4_index[("C", "baseline", 102)]
    diagnostic_variants = ["baseline", "failure_disabled", "detection_only", "hello_only", "tc_only", "full_adaptive"]
    for variant in diagnostic_variants:
        if variant == "baseline":
            row = baseline
        elif variant == "full_adaptive":
            row = g4_index[("C", "adaptive", 102)]
        else:
            row = g6_index[("C", variant, 102)]
        writer.writerow([variant, row["pdr_percent"], row["normalized_overhead"], row["recovery_seconds"],
                         row["failure_transitions"], float(row["pdr_percent"]) - float(baseline["pdr_percent"])])

with (output_arg / "g6_trigger_summary.csv").open("w", newline="", encoding="utf-8") as handle:
    writer = csv.writer(handle)
    writer.writerow(["scenario", "variant", "n_runs", "runs_with_any_trigger",
                     "signal_transitions", "failure_transitions", "recoveries"])
    for scenario, scenario_variants in variants.items():
        for variant in scenario_variants:
            selected = [variant_row(scenario, variant, run) for run in runs]
            writer.writerow(
                [scenario, variant, len(selected),
                 sum(float(r["signal_transitions"]) + float(r["failure_transitions"]) > 0 for r in selected),
                 int(sum(float(r["signal_transitions"]) for r in selected)),
                 int(sum(float(r["failure_transitions"]) for r in selected)),
                 int(sum(float(r["recoveries"]) for r in selected))]
            )

with (output_arg / "g6_leave_one_out_pdr.csv").open("w", newline="", encoding="utf-8") as handle:
    writer = csv.writer(handle)
    writer.writerow(["scenario", "variant", "omitted_run", "mean_pdr_difference"])
    for scenario, scenario_variants in variants.items():
        for variant in scenario_variants:
            values = differences(scenario, variant, "pdr_percent")
            for position, run in enumerate(runs):
                writer.writerow([scenario, variant, run, f"{np.mean(np.delete(values, position)):.9f}"])

plt.rcParams.update({"font.size": 9, "axes.titlesize": 10, "axes.labelsize": 9,
                     "legend.fontsize": 8, "figure.dpi": 150, "savefig.dpi": 300})

component_variants = ["detection_only", "hello_only", "tc_only", "full_adaptive"]
labels = ["Detection only", "HELLO only", "TC only", "Full adaptive"]
fig, axes = plt.subplots(1, 2, figsize=(7.2, 3.1))
for axis, metric, title, unit in [
    (axes[0], "pdr_percent", "Service effect", "PDR difference (pp)"),
    (axes[1], "normalized_overhead", "Control cost", "Normalized overhead difference"),
]:
    means, lows, highs = [], [], []
    for variant in component_variants:
        values = differences("C", variant, metric)
        ci = bootstrap_ci(values, np.mean)
        means.append(np.mean(values)); lows.append(np.mean(values) - ci[0]); highs.append(ci[1] - np.mean(values))
    axis.errorbar(range(len(labels)), means, yerr=[lows, highs], fmt="o", capsize=3, color="#2463a6")
    axis.axhline(0, color="black", linewidth=0.8)
    axis.set_xticks(range(len(labels)), labels, rotation=20, ha="right")
    axis.set_ylabel(unit); axis.set_title(title); axis.grid(axis="y", alpha=0.25)
fig.suptitle("Scenario C component ablation: paired mean effects with 95% bootstrap CI", y=1.02)
fig.tight_layout()
fig.savefig(figures_arg / "g6_component_ablation.png", bbox_inches="tight")
fig.savefig(figures_arg / "g6_component_ablation.pdf", bbox_inches="tight")
plt.close(fig)

fig, axes = plt.subplots(1, 2, figsize=(7.2, 3.1))
for axis, scenario, ordered, labels_, title in [
    (axes[0], "B", ["signal78", "full_adaptive", "signal84"], ["−78", "−81", "−84"], "Signal threshold (dBm)"),
    (axes[1], "C", ["failure20", "full_adaptive", "failure80"], ["20", "40", "80"], "Failure-event threshold"),
]:
    means, lows, highs = [], [], []
    for variant in ordered:
        values = differences(scenario, variant, "pdr_percent")
        ci = bootstrap_ci(values, np.mean)
        means.append(np.mean(values)); lows.append(np.mean(values) - ci[0]); highs.append(ci[1] - np.mean(values))
    axis.errorbar(range(3), means, yerr=[lows, highs], fmt="o-", capsize=3, color="#c05a2b")
    axis.axhline(0, color="black", linewidth=0.8)
    axis.set_xticks(range(3), labels_); axis.set_xlabel(title); axis.grid(axis="y", alpha=0.25)
axes[0].set_ylabel("Mean PDR difference vs baseline (pp)")
fig.suptitle("Threshold sensitivity: paired mean PDR effects with 95% bootstrap CI", y=1.02)
fig.tight_layout()
fig.savefig(figures_arg / "g6_threshold_sensitivity.png", bbox_inches="tight")
fig.savefig(figures_arg / "g6_threshold_sensitivity.pdf", bbox_inches="tight")
plt.close(fig)

expected = 141
unique = {(r["scenario"], r["variant"], int(r["run"])) for r in g6}
network_fields = ["tx_packets", "rx_packets", "lost_packets", "pdr_percent",
                  "min_flow_pdr_percent", "mean_delay_ms", "delay_p50_ms", "delay_p95_ms",
                  "goodput_kbps", "olsr_control_packets", "olsr_control_bytes",
                  "normalized_overhead", "recovery_seconds", "next_hop_changes",
                  "route_unavailable_samples", "disrupted_node"]
detection_matches = all(
    variant_row("C", "detection_only", run)[field] == g4_index[("C", "baseline", run)][field]
    for run in runs for field in network_fields
)
hello_matches = all(
    variant_row("C", "hello_only", run)[field] == g4_index[("C", "adaptive", run)][field]
    for run in runs for field in network_fields
)
status = "PASS" if (len(g6) == expected and len(unique) == expected and
                    detection_matches and hello_matches) else "FAIL"
with (output_arg / "g6_validation.txt").open("w", encoding="utf-8") as handle:
    handle.write(f"G6_ROWS={len(g6)}\nG6_UNIQUE_ROWS={len(unique)}\n"
                 f"DETECTION_ONLY_EQUALS_BASELINE={detection_matches}\n"
                 f"HELLO_ONLY_EQUALS_FULL_ADAPTIVE={hello_matches}\nG6_VALIDATION={status}\n")
if status != "PASS":
    raise SystemExit("G6 structural validation failed")
