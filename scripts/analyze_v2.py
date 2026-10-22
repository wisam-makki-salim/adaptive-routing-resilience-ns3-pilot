#!/usr/bin/env python3
"""Validate and analyze the fresh three-baseline Pilot Study v2 experiment."""

import csv
import hashlib
import json
import pathlib
import sys

import matplotlib.pyplot as plt
import numpy as np

raw_path = pathlib.Path(sys.argv[1])
output_dir = pathlib.Path(sys.argv[2])
figures_dir = pathlib.Path(sys.argv[3])
output_dir.mkdir(parents=True, exist_ok=True)
figures_dir.mkdir(parents=True, exist_ok=True)

with raw_path.open(newline="", encoding="utf-8") as handle:
    rows = list(csv.DictReader(handle))

scenarios = list("ABCD")
mechanisms = ["baseline", "always_on", "triggered"]
runs = list(range(201, 231))
expected = {(s, m, r) for s in scenarios for m in mechanisms for r in runs}
index = {(r["scenario"], r["mechanism"], int(r["run"])): r for r in rows}
if set(index) != expected or len(rows) != len(expected):
    missing = sorted(expected - set(index))
    extra = sorted(set(index) - expected)
    raise SystemExit(f"Invalid v2 run matrix: missing={missing[:5]} extra={extra[:5]}")

for scenario in "BCD":
    for run in runs:
        targets = {index[(scenario, mechanism, run)]["disrupted_node"] for mechanism in mechanisms}
        if len(targets) != 1 or "-1" in targets:
            raise SystemExit(
                f"Disturbance-target mismatch: scenario={scenario} run={run} targets={targets}"
            )

metrics = [
    "pdr_percent",
    "min_flow_pdr_percent",
    "delay_p95_ms",
    "goodput_kbps",
    "normalized_overhead",
    "next_hop_changes",
    "route_unavailable_samples",
]
BOOTSTRAP_SEED = 20261022
BOOTSTRAP_RESAMPLES = 50_000


def paired_values(scenario, mechanism, metric):
    return np.array(
        [
            float(index[(scenario, mechanism, run)][metric])
            - float(index[(scenario, "baseline", run)][metric])
            for run in runs
        ],
        dtype=float,
    )


def bootstrap_mean_ci(values, key):
    """Return one reproducible interval for one named estimand.

    The key-derived seed prevents the interval from depending on call order.  A
    given scenario/comparison/metric therefore has one canonical interval in
    tables, figures, and documents.
    """
    digest = hashlib.sha256(f"{BOOTSTRAP_SEED}:{key}".encode("utf-8")).digest()
    seed = int.from_bytes(digest[:8], "big")
    rng = np.random.default_rng(seed)
    samples = values[rng.integers(0, len(values), size=(BOOTSTRAP_RESAMPLES, len(values)))]
    return np.percentile(np.mean(samples, axis=1), [2.5, 97.5])


def comparison_values(scenario, left, right, metric):
    return np.array(
        [
            float(index[(scenario, left, run)][metric])
            - float(index[(scenario, right, run)][metric])
            for run in runs
        ],
        dtype=float,
    )


comparisons = [
    ("always_on_minus_static", "always_on", "baseline"),
    ("triggered_minus_static", "triggered", "baseline"),
    ("triggered_minus_always_on", "triggered", "always_on"),
]

canonical = {
    "source": str(raw_path.relative_to(raw_path.parents[2])),
    "bootstrap_seed": BOOTSTRAP_SEED,
    "bootstrap_resamples": BOOTSTRAP_RESAMPLES,
    "method": "paired percentile bootstrap of the mean",
    "estimands": {},
}
for scenario in scenarios:
    for label, left, right in comparisons:
        for metric in metrics:
            values = comparison_values(scenario, left, right, metric)
            key = f"{scenario}:{label}:{metric}"
            low, high = bootstrap_mean_ci(values, key)
            canonical["estimands"][key] = {
                "scenario": scenario,
                "comparison": label,
                "left": left,
                "right": right,
                "metric": metric,
                "n_pairs": len(values),
                "mean_difference": float(np.mean(values)),
                "mean_ci95_low": float(low),
                "mean_ci95_high": float(high),
                "median_difference": float(np.median(values)),
                "q1": float(np.percentile(values, 25)),
                "q3": float(np.percentile(values, 75)),
                "minimum": float(np.min(values)),
                "maximum": float(np.max(values)),
                "lower_count": int(np.sum(values < 0)),
                "equal_count": int(np.sum(values == 0)),
                "higher_count": int(np.sum(values > 0)),
            }

with (output_dir / "v2_canonical_summary.json").open("w", encoding="utf-8") as handle:
    json.dump(canonical, handle, indent=2, sort_keys=True)
    handle.write("\n")


with (output_dir / "v2_paired_effects.csv").open("w", newline="", encoding="utf-8") as handle:
    writer = csv.writer(handle, lineterminator="\n")
    writer.writerow(["scenario", "mechanism", "run", "metric", "difference_vs_static"])
    for scenario in scenarios:
        for mechanism in mechanisms[1:]:
            for run in runs:
                for metric in metrics:
                    value = float(index[(scenario, mechanism, run)][metric])
                    base = float(index[(scenario, "baseline", run)][metric])
                    writer.writerow([scenario, mechanism, run, metric, f"{value - base:.9f}"])

with (output_dir / "v2_effect_summary.csv").open("w", newline="", encoding="utf-8") as handle:
    writer = csv.writer(handle, lineterminator="\n")
    writer.writerow(
        [
            "scenario", "mechanism", "metric", "n_pairs", "mean_difference",
            "mean_ci95_low", "mean_ci95_high", "median_difference", "q1", "q3",
            "minimum", "maximum", "lower_count", "equal_count", "higher_count",
        ]
    )
    for scenario in scenarios:
        for mechanism in mechanisms[1:]:
            for metric in metrics:
                values = paired_values(scenario, mechanism, metric)
                record = canonical["estimands"][
                    f"{scenario}:{mechanism}_minus_static:{metric}"
                ]
                low, high = record["mean_ci95_low"], record["mean_ci95_high"]
                writer.writerow(
                    [
                        scenario, mechanism, metric, len(values), f"{np.mean(values):.9f}",
                        f"{low:.9f}", f"{high:.9f}", f"{np.median(values):.9f}",
                        f"{np.percentile(values, 25):.9f}", f"{np.percentile(values, 75):.9f}",
                        f"{np.min(values):.9f}", f"{np.max(values):.9f}",
                        int(np.sum(values < 0)), int(np.sum(values == 0)), int(np.sum(values > 0)),
                    ]
                )

with (output_dir / "v2_pairwise_comparison.csv").open(
    "w", newline="", encoding="utf-8"
) as handle:
    writer = csv.writer(handle, lineterminator="\n")
    writer.writerow(
        [
            "scenario", "comparison", "metric", "n_pairs", "mean_difference",
            "mean_ci95_low", "mean_ci95_high", "median_difference", "minimum",
            "maximum", "lower_count", "equal_count", "higher_count",
        ]
    )
    for scenario in scenarios:
        for label, left, right in comparisons:
            for metric in metrics:
                values = comparison_values(scenario, left, right, metric)
                record = canonical["estimands"][f"{scenario}:{label}:{metric}"]
                low, high = record["mean_ci95_low"], record["mean_ci95_high"]
                writer.writerow(
                    [
                        scenario, label, metric, len(values), f"{np.mean(values):.9f}",
                        f"{low:.9f}", f"{high:.9f}", f"{np.median(values):.9f}",
                        f"{np.min(values):.9f}", f"{np.max(values):.9f}",
                        int(np.sum(values < 0)), int(np.sum(values == 0)), int(np.sum(values > 0)),
                    ]
                )

with (output_dir / "v2_recovery_summary.csv").open(
    "w", newline="", encoding="utf-8"
) as handle:
    writer = csv.writer(handle, lineterminator="\n")
    writer.writerow(
        [
            "scenario", "mechanism", "n_runs", "recovered_runs", "not_recovered_runs",
            "median_recovery_seconds_among_recovered", "minimum_recovery_seconds",
            "maximum_recovery_seconds",
        ]
    )
    for scenario in "BCD":
        for mechanism in mechanisms:
            values = np.array(
                [float(index[(scenario, mechanism, run)]["recovery_seconds"]) for run in runs]
            )
            recovered = values[values >= 0]
            writer.writerow(
                [
                    scenario, mechanism, len(values), len(recovered), int(np.sum(values == -2)),
                    f"{np.median(recovered):.9f}" if len(recovered) else "NA",
                    f"{np.min(recovered):.9f}" if len(recovered) else "NA",
                    f"{np.max(recovered):.9f}" if len(recovered) else "NA",
                ]
            )

with (output_dir / "v2_trigger_summary.csv").open(
    "w", newline="", encoding="utf-8"
) as handle:
    writer = csv.writer(handle, lineterminator="\n")
    writer.writerow(
        [
            "scenario", "mechanism", "runs_with_any_trigger", "signal_transitions",
            "failure_transitions", "recoveries",
        ]
    )
    for scenario in scenarios:
        for mechanism in mechanisms:
            selected = [index[(scenario, mechanism, run)] for run in runs]
            writer.writerow(
                [
                    scenario, mechanism,
                    sum(int(r["signal_transitions"]) + int(r["failure_transitions"]) > 0 for r in selected),
                    sum(int(r["signal_transitions"]) for r in selected),
                    sum(int(r["failure_transitions"]) for r in selected),
                    sum(int(r["recoveries"]) for r in selected),
                ]
            )

plt.rcParams.update(
    {"font.size": 9, "axes.titlesize": 10, "axes.labelsize": 9, "legend.fontsize": 8,
     "figure.dpi": 150, "savefig.dpi": 300}
)

# Figure 1: executable study architecture.
fig, axis = plt.subplots(figsize=(8.0, 2.8))
axis.axis("off")
boxes = [
    (0.02, "Controlled condition\nA–D"),
    (0.27, "Paired mechanisms\nB0 / B1 / B2"),
    (0.52, "ns-3.47 execution\nrun 201–230"),
    (0.77, "Service + control\ntrade-off analysis"),
]
for x, label in boxes:
    axis.add_patch(plt.Rectangle((x, 0.34), 0.19, 0.32, fill=False, linewidth=1.2))
    axis.text(x + 0.095, 0.50, label, ha="center", va="center")
for x in (0.22, 0.47, 0.72):
    axis.annotate("", xy=(x + 0.04, 0.50), xytext=(x, 0.50),
                  arrowprops={"arrowstyle": "->", "linewidth": 1.1})
axis.set_title("Pilot Study v2 experimental workflow")
fig.tight_layout()
fig.savefig(figures_dir / "v2_experimental_architecture.png", bbox_inches="tight")
fig.savefig(figures_dir / "v2_experimental_architecture.pdf", bbox_inches="tight")
plt.close(fig)

# Figure 2: paired resilience effects with uncertainty.
fig, axis = plt.subplots(figsize=(7.2, 3.6))
offsets = {"always_on": -0.08, "triggered": 0.08}
styles = {"always_on": ("o", "#8b4a3b", "Always-on"),
          "triggered": ("s", "#1f5f8b", "State-triggered")}
for mechanism in mechanisms[1:]:
    marker, color, label = styles[mechanism]
    means, lows, highs = [], [], []
    for scenario in scenarios:
        values = paired_values(scenario, mechanism, "pdr_percent")
        record = canonical["estimands"][
            f"{scenario}:{mechanism}_minus_static:pdr_percent"
        ]
        low, high = record["mean_ci95_low"], record["mean_ci95_high"]
        mean = np.mean(values)
        means.append(mean); lows.append(mean - low); highs.append(high - mean)
    x = np.arange(len(scenarios)) + offsets[mechanism]
    axis.errorbar(x, means, yerr=[lows, highs], fmt=marker, capsize=3,
                  color=color, label=label)
axis.axhline(0, color="black", linewidth=0.8)
axis.set_xticks(range(len(scenarios)), ["Stable", "Progressive", "Failure", "Repeated"])
axis.set_ylabel("Mean paired PDR difference vs static (pp)")
axis.set_title("Resilience effect relative to static OLSR")
axis.grid(axis="y", alpha=0.25)
axis.legend(frameon=False)
fig.tight_layout()
fig.savefig(figures_dir / "v2_resilience_performance.png", bbox_inches="tight")
fig.savefig(figures_dir / "v2_resilience_performance.pdf", bbox_inches="tight")
plt.close(fig)

# Figure 3: mean benefit versus control cost, with paired uncertainty.
fig, axis = plt.subplots(figsize=(6.2, 4.3))
annotation_offsets = {
    ("always_on", "A"): (5, 7),
    ("always_on", "B"): (5, 7),
    ("always_on", "C"): (5, 7),
    ("always_on", "D"): (5, 7),
    ("triggered", "A"): (6, 10),
    ("triggered", "B"): (7, -2),
    ("triggered", "C"): (7, -15),
    ("triggered", "D"): (-1, 10),
}
for mechanism in mechanisms[1:]:
    marker, color, label = styles[mechanism]
    for position, scenario in enumerate(scenarios):
        pdr = paired_values(scenario, mechanism, "pdr_percent")
        overhead = paired_values(scenario, mechanism, "normalized_overhead")
        pdr_record = canonical["estimands"][
            f"{scenario}:{mechanism}_minus_static:pdr_percent"
        ]
        overhead_record = canonical["estimands"][
            f"{scenario}:{mechanism}_minus_static:normalized_overhead"
        ]
        pdr_ci = np.array([pdr_record["mean_ci95_low"], pdr_record["mean_ci95_high"]])
        overhead_ci = np.array(
            [overhead_record["mean_ci95_low"], overhead_record["mean_ci95_high"]]
        )
        axis.errorbar(
            np.mean(overhead), np.mean(pdr),
            xerr=[[np.mean(overhead) - overhead_ci[0]], [overhead_ci[1] - np.mean(overhead)]],
            yerr=[[np.mean(pdr) - pdr_ci[0]], [pdr_ci[1] - np.mean(pdr)]],
            fmt=marker, color=color, capsize=2, label=label if position == 0 else None,
        )
        axis.annotate(scenario, (np.mean(overhead), np.mean(pdr)),
                      xytext=annotation_offsets[(mechanism, scenario)],
                      textcoords="offset points", fontsize=8)
axis.axhline(0, color="black", linewidth=0.8)
axis.axvline(0, color="black", linewidth=0.8)
axis.set_xlabel("Mean normalized-overhead difference vs static")
axis.set_ylabel("Mean PDR difference vs static (pp)")
axis.set_title("Adaptation benefit versus control cost")
axis.grid(alpha=0.25)
axis.legend(frameon=False)
fig.tight_layout()
fig.savefig(figures_dir / "v2_benefit_cost_tradeoff.png", bbox_inches="tight")
fig.savefig(figures_dir / "v2_benefit_cost_tradeoff.pdf", bbox_inches="tight")
plt.close(fig)

with (output_dir / "v2_validation.txt").open("w", encoding="utf-8") as handle:
    handle.write(
        f"V2_ROWS={len(rows)}\nV2_UNIQUE_ROWS={len(index)}\n"
        "V2_EXPECTED_ROWS=360\nV2_FRESH_RUNS=201-230\nV2_VALIDATION=PASS\n"
    )
