# G2 Baseline Report

## Decision

**G2 = PASS**

This gate establishes a functioning fixed-parameter OLSR baseline for Scenario A. Runs 1-5 are development runs and are not part of the locked final evaluation set (runs 101-120).

## Design executed

- 16-node 4x4 IEEE 802.11g ad hoc grid.
- 60 m nominal spacing with seeded +/-2 m position jitter.
- 75 m communication range, yielding cardinal-neighbor multi-hop connectivity.
- Fixed OLSRv1 configuration: HELLO 2 s and TC 5 s.
- Two simultaneous UDP flows: nodes 0 to 15 and nodes 3 to 12.
- 512-byte payload every 20 ms per flow.
- 20 s routing warm-up, traffic until 110 s, simulation stop at 120 s.
- Common fixed seed with independent development run numbers.

## Development-run summary

| Metric | Median | Q1-Q3 | Range |
|---|---:|---:|---:|
| Aggregate PDR | 98.817% | 96.626-98.954% | 93.786-99.588% |
| Worst-flow PDR | 97.907% | 94.839-98.014% | 93.284-99.265% |
| Delay p50 | 6 ms | 6-6 ms | 6-8 ms |
| Delay p95 | 9 ms | 7-16 ms | 6-17 ms |
| Aggregate goodput | 403.137 kbps | 394.945-404.639 | 383.977-406.687 |
| OLSR control packets during traffic window | 1,158 | 1,157-1,158 | 1,155-1,196 |
| Normalized control overhead | 0.0193 B/B | 0.0190-0.0193 | 0.0185-0.0194 |
| Observed next-hop changes | 12 | 10-12 | 5-13 |
| Route-unavailable samples | 0 | 0-0 | 0-0 |

## Interpretation

The baseline is operational: both flows are delivered in every run, no sampled source-to-destination route is unavailable after warm-up, and all required measurement paths work. Variation across seeds is material even without planned mobility or failure. In particular, aggregate PDR ranges from 93.8% to 99.6%, so a single-run comparison would be methodologically invalid.

The next-hop tracker records 5-13 changes despite fixed node positions. These are actual changes in the OLSR next hop for the two monitored destinations, not the much noisier `RoutingTableChanged` callback. The result suggests equal-cost route switching and wireless/MPR dynamics and creates a meaningful route-stability baseline for later gates.

Mean delay is larger than p95 in some runs because a small extreme tail can dominate the mean. The locked report therefore prioritizes p50 and p95; mean delay remains diagnostic only.

## Measurement correction made during G2

The initial implementation counted the ns-3 `RoutingTableChanged` trace. It produced thousands of callbacks in a static scenario and was rejected as a proxy for path changes. The final implementation samples the actual OLSR next hop for each monitored destination every 250 ms and counts only changes in that next hop. Superseded callback counts are not retained as evidence.

OLSR control overhead is counted only in the 20-110 s traffic observation window. It is reported both as transmitted control packets/bytes and control bytes per delivered application-payload byte.

## Reproducibility check

Development run 3 was executed twice independently and produced an identical output row byte for byte.

## Limitations at this gate

- The range propagation model is deliberately controlled but not physically rich.
- The five runs are engineering validation, not inferential evidence.
- Scenario A provides no evidence of resilience under degradation or failure.
- No adaptive mechanism has been implemented or tested.
- Recovery time is undefined in Scenario A because no disruption occurs.

## Gate boundary

G2 contains only fixed-parameter OLSR under Scenario A. The next authorized stage is G3: implement the lightweight adaptive mechanism and verify its trigger/timer behavior without running the final three-scenario experiment.
