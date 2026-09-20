# G6 — Robustness and Ablation

## Decision

**G6 = PASS (robustness diagnosis completed; the original mechanism is not validated).**

The analysis used the locked G4 seeds 101–120 and retained every run. It added 141 simulations defined in `configs/g6_robustness.yaml`: 140 paired sensitivity/ablation runs plus one failure-disabled diagnostic for C/102. No result was removed and no threshold was added after observing G6 outcomes.

## 1. Component ablation: Scenario C

All values are paired differences against the G4 baseline. Intervals are deterministic 95% percentile bootstrap intervals with 50,000 resamples.

| Variant | Mean PDR difference, pp (95% CI) | Mean overhead difference (95% CI) | Trigger behavior | Interpretation |
|---|---:|---:|---|---|
| Detection only | 0.000 (0.000, 0.000) | 0.000000 (0.000000, 0.000000) | Failure trigger in 20/20 | Detection alone has no network effect |
| HELLO only | -0.659 (-2.784, +0.600) | +0.001292 (+0.001008, +0.001779) | Failure trigger in 20/20 | Reproduces the full mechanism |
| TC only | +0.117 (0.000, +0.351) | -0.000011 (-0.000045, +0.000012) | Failure trigger in 20/20 | Nearly identical to baseline; one improved pair |
| Full adaptive | -0.659 (-2.782, +0.609) | +0.001292 (+0.001009, +0.001780) | Failure trigger in 20/20 | No robust benefit; costly adverse tail |

The full mechanism and HELLO-only variant produce the same aggregate results for all reported outcomes. Detection-only is exactly equal to baseline in all 20 pairs. Thus, the harmful and costly behavior is attributable to shortening the OLSR HELLO interval from 2.0 s to 0.5 s, not to event detection itself and not to the TC interval change.

### C/102 causal check

| Variant | PDR (%) | Overhead | Recovery (s) | PDR difference vs baseline |
|---|---:|---:|---:|---:|
| Baseline | 95.512749 | 0.017424 | 7 | 0.000000 |
| Failure trigger disabled | 95.512749 | 0.017424 | 7 | 0.000000 |
| Detection only | 95.512749 | 0.017424 | 7 | 0.000000 |
| HELLO only | 76.338938 | 0.023060 | 36 | -19.173811 |
| TC only | 95.512749 | 0.017424 | 7 | 0.000000 |
| Full adaptive | 76.338938 | 0.023060 | 36 | -19.173811 |

This is strong within-simulation causal evidence that accelerated HELLO messaging creates the adverse behavior in run 102. It does not yet identify the lower-level cause—such as route-state interaction or wireless contention—and should not be generalized beyond the tested configuration.

## 2. Failure-threshold sensitivity: Scenario C

| Failure threshold | Mean PDR difference, pp (95% CI) | Mean recovery difference, s (95% CI) | Mean overhead difference | Triggered runs |
|---:|---:|---:|---:|---:|
| 20 | +0.434 (+0.082, +0.884) | -0.450 (-0.950, -0.050) | +0.001082 | 20/20 |
| 40 | -0.659 (-2.782, +0.609) | +1.100 (-0.750, +4.250) | +0.001292 | 20/20 |
| 80 | 0.000 (0.000, 0.000) | 0.000 (0.000, 0.000) | 0.000000 | 0/20 |

Threshold 20 is the only tested variant with a positive mean PDR interval and a negative recovery-time interval. Its leave-one-out mean PDR effect remains positive (+0.277 to +0.482 pp). This is a credible candidate for a redesigned mechanism, but it is **exploratory evidence**, not confirmation: it was evaluated after the original locked experiment and on the same topology family. Threshold 80 simply disables adaptation under the tested failure, so equality with baseline is not resilience evidence.

## 3. Signal-threshold sensitivity: Scenario B

| Risk threshold | Mean PDR difference, pp (95% CI) | Mean overhead difference | Transition pattern |
|---:|---:|---:|---|
| -78 dBm | +1.123 (-0.018, +1.839) | +0.048785 | 317 signal transitions across 20 runs |
| -81 dBm | +0.057 (-0.118, +0.314) | +0.002518 | 39 signal transitions across 20 runs |
| -84 dBm | -1.333 (-4.146, +0.120) | +0.001533 | 0 signal and 19 failure transitions |

The relationship is not monotonic evidence of an optimal threshold. At -78 dBm the controller becomes highly reactive, generating 317 transitions and a large overhead increase; the modest PDR gain is operationally expensive and its interval still touches zero. At -84 dBm, suppressing signal triggers allows the alternative failure trigger to dominate in 19 runs. This trigger substitution means the result is a coupled-controller sensitivity test, not an isolated RSSI-response curve.

## 4. Robustness conclusion

1. The original full adaptive mechanism does not support a superiority claim.
2. The main design error is unconditional HELLO acceleration after a trigger; it can cause network-wide harm to another flow.
3. Detection is not the problem: detection-only produces baseline-equivalent behavior.
4. TC acceleration alone adds no consistent cost or service benefit in this configuration.
5. Failure threshold 20 is a promising redesign candidate, but adopting it as “the answer” from these same runs would be threshold overfitting.
6. A defensible next mechanism should gate HELLO acceleration using congestion/route-state safeguards, cap its duration, and be evaluated on fresh seeds or a second topology.

## Artifacts

- `results/g6/raw/g6_runs.csv`: all 141 new runs.
- `results/g6/g6_paired_uncertainty.csv`: paired effects and bootstrap intervals.
- `results/g6/g6_trigger_summary.csv`: trigger-channel behavior.
- `results/g6/g6_leave_one_out_pdr.csv`: influence analysis.
- `results/g6/g6_run102_ablation.csv`: causal diagnostic.
- `figures/g6_component_ablation.{png,pdf}`.
- `figures/g6_threshold_sensitivity.{png,pdf}`.

## Gate boundary

G6 is complete. The technical note and final research package are documented in `docs/G7_RESEARCH_PACKAGE_REPORT.md`.
