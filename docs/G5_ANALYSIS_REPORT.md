# G5 — Paired Analysis and Uncertainty

## Decision

**G5 = PASS (analysis completeness, not hypothesis confirmation).**

The locked 120-run G4 dataset was analyzed without deleting runs, changing seeds, or retuning the mechanism. The unit of analysis is the paired difference for the same scenario and ns-3 run number:

`adaptive − baseline`

All intervals are deterministic percentile bootstrap intervals with 50,000 resamples and bootstrap seed 20260920. With only 20 pairs per scenario, the intervals are descriptive uncertainty estimates rather than a claim of population-level generality.

## Primary results

| Scenario | Outcome | Mean paired difference (95% bootstrap CI) | Median paired difference (95% bootstrap CI) | Direction across 20 pairs |
|---|---|---:|---:|---:|
| A | PDR (percentage points) | 0.000 (0.000, 0.000) | 0.000 (0.000, 0.000) | 0 lower / 20 equal / 0 higher |
| B | PDR (percentage points) | +0.057 (-0.119, +0.312) | -0.006 (-0.022, 0.000) | 10 lower / 6 equal / 4 higher |
| B | Normalized overhead | +0.002518 (+0.002102, +0.002946) | +0.002541 (+0.001667, +0.003179) | 0 lower / 0 equal / 20 higher |
| B | Recovery time (s) | -0.050 (-0.150, 0.000) | 0.000 (0.000, 0.000) | 1 lower / 19 equal / 0 higher |
| C | PDR (percentage points) | -0.659 (-2.777, +0.604) | 0.000 (-0.007, +0.279) | 7 lower / 4 equal / 9 higher |
| C | Normalized overhead | +0.001292 (+0.001009, +0.001781) | +0.001108 (+0.001060, +0.001132) | 0 lower / 0 equal / 20 higher |
| C | Recovery time (s) | +1.100 (-0.750, +4.250) | 0.000 (-0.500, 0.000) | 6 lower / 11 equal / 3 higher |

PDR and goodput tell the same substantive story because packet size and offered load are fixed. P95 delay differences are mostly zero and their intervals include zero. Reporting these correlated outcomes as separate independent successes would be misleading.

## Interpretation by scenario

### Scenario A — normal operation

Baseline and adaptive results are identical for every reported metric in all 20 paired runs. No adaptive trigger fired. This is a useful negative control: the controller does not add overhead when it remains in the stable state.

### Scenario B — gradual mobility/link degradation

The PDR interval crosses zero, the median difference is slightly negative, and adaptive PDR is lower in 10 pairs but higher in only 4. Recovery differs in only one pair. In contrast, normalized overhead is higher in all 20 adaptive runs and its interval is wholly above zero. Therefore, the current mechanism shows a repeatable cost without repeatable service-resilience benefit under gradual degradation.

### Scenario C — node failure

The median PDR effect is zero and 9 of 20 pairs improve, but uncertainty includes both benefit and harm. The negative mean is driven strongly by the retained adverse-tail run C/102. Recovery-time uncertainty also crosses zero, while overhead is higher in every adaptive pair. The current evidence does not support a robust improvement claim.

## Run C/102 diagnostic

The run was rerun with the locked seed and configuration. Its aggregate result matches the G4 row exactly:

| Metric | Baseline | Adaptive | Difference |
|---|---:|---:|---:|
| PDR (%) | 95.512749 | 76.338938 | -19.173811 pp |
| Worst-flow PDR (%) | 94.372776 | 58.060201 | -36.312575 pp |
| Goodput (kbps) | 390.394311 | 312.024178 | -78.370133 |
| Normalized overhead | 0.017424 | 0.023060 | +0.005636 |
| Recovery time (s) | 7 | 36 | +29 |

Both mechanisms changed flow 0's source next hop at 64.5 s after node 6 failed at 60 s. The adaptive controller entered reactive state at 62.121085 s after final-transmission failures. Per-flow traces show that flow 0 resumed in both cases, while flow 1 remained largely undelivered in the adaptive run even though its source retained a route. The present traces establish a cross-flow/downstream routing side effect but do not uniquely identify whether the cause is transient topology inconsistency, interference from increased control traffic, or another OLSR interaction. Causal isolation is deliberately deferred to G6 rather than asserted from insufficient evidence.

## Evidence boundary

- The hypothesis is **not supported** by the locked evaluation: benefit is not stable across seeds, while overhead increases consistently whenever adaptation triggers.
- This is not evidence that adaptive routing is generally ineffective. It is evidence about this lightweight timer-switching mechanism, topology, traffic pattern, and disruption model.
- Bootstrap intervals quantify variability among the 20 locked paired runs; they do not compensate for testing only one topology family.
- No multiple-comparison significance claims are made.
- The adverse-tail run is retained and highlighted, not treated as removable noise.

## Generated evidence

- `results/g5/g5_paired_uncertainty.csv`: mean/median paired effects and 95% bootstrap intervals.
- `results/g5/g5_direction_counts.csv`: lower/equal/higher counts without assuming normality.
- `results/g5/g5_run102_comparison.csv`: exact adverse-tail comparison.
- `results/g5/traces/`: per-second and per-flow receive traces, route events, controller transitions, and rerun rows.
- `figures/g5_paired_pdr_differences.{png,pdf}`
- `figures/g5_overhead_pdr_tradeoff.{png,pdf}`
- `figures/g5_run102_rx_timeline.{png,pdf}`

## Gate boundary

G5 analyzes the locked experiment only. Threshold sensitivity and causal ablation belong to G6 and have not been performed here.
