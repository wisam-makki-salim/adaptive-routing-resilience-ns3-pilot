# Pilot Study v2 Results

## Execution status

The v2 experiment executed successfully in ns-3.47. It contains 360 fresh runs:
four conditions, three mechanisms, and runs 201–230. The analysis found 360
unique result keys and no missing rows. For every paired comparison in conditions
B, C, and D, the disturbed node was fixed from the static B0 route and was the
same under B0, B1, and B2.

## Mechanisms

- B0 static: OLSR HELLO 2.0 s and TC 5.0 s.
- B1 always-on: HELLO 0.5 s and TC 1.0 s for the whole run.
- B2 state-triggered: the faster intervals begin after the locked signal or
  failure trigger. The failure threshold was fixed at 20 events before these
  fresh runs.

## Main estimates

All PDR values below are paired mean differences from B0 in percentage points.
Intervals are deterministic 95% percentile-bootstrap intervals over 30 paired
runs. They quantify variation in this simulation design and do not establish
generality outside it.

| Condition | B1 PDR difference | B2 PDR difference | B1 overhead difference | B2 overhead difference |
|---|---:|---:|---:|---:|
| A stable | +0.571 [-0.630, +2.312] | +0.432 [+0.000, +1.296] | +0.05410 | +0.00004 |
| B progressive | +1.591 [+1.214, +1.971] | +0.108 [-0.069, +0.320] | +0.05305 | +0.00307 |
| C failure | +1.453 [+1.055, +1.796] | -0.117 [-0.279, +0.010] | +0.05043 | +0.00111 |
| D repeated | +3.392 [+2.736, +4.049] | +0.404 [-0.261, +1.055] | +0.05357 | +0.00378 |

## Findings

**B1 improves delivery under the three disturbance conditions, but its control
cost is large and consistent.** B1 had higher PDR than B0 in 28/30 progressive
runs, 28/30 failure runs, and 29/30 repeated-disturbance runs. Mean normalized
overhead increased by approximately 0.050–0.054 in every condition. The p95 delay
also increased in 26/30 progressive runs, 28/30 failure runs, and 28/30 repeated
runs.

**The locked B2 rule did not show a repeatable delivery advantage.** Its PDR
interval crossed zero in B and D. In C the mean was negative, with 9 lower, 13
equal, and 8 higher pairs. B2 reduced normalized overhead by about 0.049–0.050
relative to B1 in every disturbance condition, but it also delivered less PDR
than B1 in 28/30 B pairs, 28/30 C pairs, and 28/30 D pairs.

**The stable negative control exposed false triggering.** B2 triggered in 3/30
stable runs, with four failure-trigger transitions and no recovery transition.
Only one stable pair changed its service result, producing a large positive
outlier; 29/30 PDR pairs were identical. The mean is therefore not evidence of a
stable-condition benefit. The 20-event failure threshold is too sensitive for
the intended negative-control behavior.

**Worst-flow results support the same conclusion.** Relative to B0, B1 mean
minimum-flow PDR increased by 3.697 points in B, 3.349 in C, and 7.229 in D. B2
mean changes were +0.217, -0.220, and +0.475 respectively, with intervals that
crossed zero. Relative to B1, B2 minimum-flow PDR was lower by 3.480 points in B,
3.570 in C, and 6.754 in D.

**Recovery requires separate reporting.** The simulator uses -2 to encode no
recovery. These values were excluded from time averages and reported as outcomes.
All B and C runs met the operational recovery definition. In D, B1 failed to meet
it in 6/30 runs; B0 and B2 met it in all 30. Conditional medians were 0 s for D
because the post-event aggregate reception already met the threshold at the
measurement boundary. This exposes a limitation of the current recovery metric.

## Interpretation boundary

The evidence does not support the proposed 20-event B2 controller as the preferred
mechanism. B1 shows that faster control can improve delivery in this topology,
but it pays a consistent overhead and delay cost and has six D runs without
operational recovery. The unresolved research problem is to design a trigger and
exit policy that captures part of B1's disturbance benefit without false triggers,
prolonged reactive states, or the full always-on cost.

## Reproducibility

The locked configuration is in `configs/v2_experiment.yaml`. Execution uses
`scripts/run_v2_experiments.sh`; `scripts/analyze_v2.py` produces one canonical
JSON summary, the analysis tables, and all three figures. The key-derived
bootstrap seed gives each named estimand one interval independent of call order.
Raw rows, per-run logs, target anchors, pairwise effects, recovery summaries, and
trigger summaries are retained under `results/v2/`.
