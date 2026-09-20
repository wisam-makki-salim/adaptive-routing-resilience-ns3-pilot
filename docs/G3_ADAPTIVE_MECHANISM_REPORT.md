# G3 Adaptive Mechanism Report

## Decision

**G3 = PASS**

The lightweight Link-Risk-Aware OLSR (LRA-OLSR) controller has been implemented and its state transitions, ns-3 trace bindings, timer mutations, hysteresis, and recovery hold-down have been validated. No comparative performance experiment was run in this gate.

## Implemented mechanism

The controller has two states:

- `STABLE`: OLSR HELLO 2.0 s and TC 5.0 s.
- `REACTIVE`: OLSR HELLO 0.5 s and TC 1.0 s.

It consumes two local inputs:

1. Per-neighbor EWMA signal level from `WifiPhy::MonitorSnifferRx`.
2. Consecutive final transmission failures from `WifiRemoteStationManager::MacTxFinalDataFailed`.

Signal measurements are maintained separately for each transmitting MAC address. A single node-wide EWMA was explicitly rejected because it could mask a degrading link or allow unrelated neighbors to dominate the trigger.

## Locked controller parameters

| Parameter | Value |
|---|---:|
| EWMA alpha | 0.3 |
| Risk threshold | -81 dBm |
| Recovery threshold | -75 dBm |
| Consecutive weak samples | 3 |
| Final Tx failures in 1 s burst | 40 |
| Recovery hold-down | 10 s |
| Hysteresis | 6 dB |

These are pre-evaluation engineering values, not optimized values and not claimed to be generally optimal. The original -78/-72 dBm pair was rejected during development calibration because it caused all 16 nodes to enter REACTIVE even in Scenario A, effectively reducing the mechanism to permanent fast-timer OLSR. No run from that rejected calibration is part of the final evaluation.

## Validation evidence

The deterministic validation sequence produced:

1. `STABLE -> REACTIVE` at 3.4 s after repeated weak per-neighbor signal samples.
2. `REACTIVE -> STABLE` after recovery above -75 dBm and completion of the 10 s hold-down.
3. `STABLE -> REACTIVE` after forty final transmission failures within one second.

At the first transition, the controller changed HELLO/TC to 0.5/1.0 s. After recovery it restored 2.0/5.0 s. Both attribute states were read back from the live OLSR object and verified.

The real Wi-Fi trace binding delivered 28 `MonitorSnifferRx` samples to a test controller and caused a test-threshold transition, establishing that the implementation is connected to ns-3 runtime traces rather than only to synthetic function calls.

## Ablation support

The controller accepts `allowReversion=false`. This implements the locked No-Reversion ablation: after the first trigger, the controller remains in fast-timer mode. It will be used only in G6.

## Important limitation

Per-neighbor measurement fixes cross-neighbor averaging, but the current controller treats any repeatedly observed weak neighbor as locally relevant. It does not yet prove that the neighbor is the current forwarding next hop for one of the measured flows. This is a conservative trigger and may increase overhead unnecessarily in dense topologies. G4 must log the triggering MAC and concurrent route next hop so this behavior is observable; it must not be hidden or described as forwarding-path-specific without evidence.

## Gate boundary

G3 establishes functional adaptation only. It provides no evidence that LRA-OLSR improves PDR, recovery time, delay, goodput, or route stability. Those claims require paired controlled experiments beginning in G4.
