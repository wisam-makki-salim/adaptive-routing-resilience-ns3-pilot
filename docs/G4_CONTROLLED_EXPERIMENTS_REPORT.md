# G4 Controlled Experiments Report

## Decision

**G4 = PASS**

This decision means that the locked controlled experiment was executed completely and the raw evidence passed structural quality checks. It does not mean that the adaptive mechanism outperformed the baseline.

## Completed experiment

- 3 scenarios: A normal, B progressive mobility/link degradation, C sudden active-relay failure.
- 2 mechanisms: fixed-parameter OLSR and LRA-OLSR.
- 20 paired final runs per cell: ns-3 run numbers 101-120.
- Total: 120 successful simulations.
- No missing rows, missing cells, duplicate experiment keys, or baseline/adaptive disruption-target mismatches.

For B and C, the affected node is the actual source-0 next hop at the event time, not a hard-coded node that might be absent from the forwarding path. The selected node is recorded in every raw row. Baseline and adaptive selected the same disrupted node for every paired seed.

## Controller calibration boundary

Calibration used development runs only. Two rejected configurations were not included in final results:

1. -78/-72 dBm caused all 16 nodes to enter REACTIVE in Scenario A.
2. Small final-failure burst thresholds produced false triggers under normal contention.

The final locked configuration uses -81/-75 dBm signal thresholds and 40 final Tx failures within one second. After locking, runs 101-120 were executed without parameter changes.

## Structural validation

| Check | Result |
|---|---|
| Scenario A adaptive triggers | 0 across all 20 runs |
| Scenario A paired outputs | Exactly identical baseline/adaptive metrics |
| Scenario B signal triggers | 1-3 per adaptive run |
| Scenario B failure triggers | 0 |
| Scenario C signal triggers | 0 |
| Scenario C failure triggers | Exactly 1 per adaptive run |
| Paired disrupted-node mismatches | 0 |

These checks show that the controller activates under the intended stressor and remains inactive in the normal scenario.

## Descriptive results

| Scenario | Mechanism | Median PDR | Median worst-flow PDR | Median p95 delay | Median goodput | Median normalized overhead | Median recovery |
|---|---|---:|---:|---:|---:|---:|---:|
| A | Baseline | 100.000% | 100.000% | 2 ms | 408.257 kbps | 0.017394 | N/A |
| A | Adaptive | 100.000% | 100.000% | 2 ms | 408.257 kbps | 0.017394 | N/A |
| B | Baseline | 97.098% | 94.331% | 2 ms | 396.447 kbps | 0.017475 | 0 s |
| B | Adaptive | 97.063% | 94.255% | 2 ms | 396.334 kbps | 0.020123 | 0 s |
| C | Baseline | 97.075% | 94.341% | 2 ms | 396.607 kbps | 0.016947 | 5 s |
| C | Adaptive | 97.210% | 94.608% | 2 ms | 396.925 kbps | 0.018016 | 5 s |

## What can and cannot be concluded at G4

Scenario A confirms that the controller does not alter normal-operation behavior after calibration. Scenario B shows no descriptive central-tendency benefit and adds control overhead. Its recovery result is mostly zero because aggregate delivery remained above the predefined 90% threshold for the first five post-event one-second windows; therefore recovery time is not discriminative for B under this load and topology.

Scenario C shows a small favorable shift in medians but no median recovery-time improvement and higher overhead. One adaptive run (run 102) is a severe adverse outlier: PDR fell from 95.51% under baseline to 76.34% under adaptation. This outlier is retained, not discarded. G5 must investigate it and quantify paired uncertainty before any performance claim.

No superiority claim is justified at G4. The current evidence is consistent with a mechanism whose effect is condition-dependent and whose faster control traffic can sometimes add harmful contention.

## Files

- `results/g4/raw/g4_final_runs.csv`: authoritative 120-run raw table.
- `results/g4/g4_summary.csv`: descriptive medians, quartiles, and ranges.
- `results/g4/g4_paired_differences.csv`: per-seed adaptive-minus-baseline differences.
- `results/g4/g4_qc.txt`: structural quality checks.

## Gate boundary

G5 must calculate paired uncertainty, inspect the run-102 failure mode, and distinguish typical effect from tail risk. No parameter may be changed in response to the final results.
