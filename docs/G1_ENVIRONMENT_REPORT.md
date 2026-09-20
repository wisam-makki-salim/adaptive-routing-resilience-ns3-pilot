# G1 Environment Report

## Decision

**G1 = PASS**

Execution date: 2026-09-20 (UTC)

## Verified environment

| Component | Verified value |
|---|---|
| Operating system | Ubuntu 24.04 environment |
| Compiler | GCC/G++ 13.3.0 |
| Build system | CMake 4.4.3 and Ninja 1.13.2 |
| Simulator | ns-3.47, release profile |
| Source archive SHA-256 | `577da01df6386b57ce362b7b82b8d3bdab7f748bc0836ba51249c8dae743ff2e` |
| Required modules | core, network, internet, mobility, Wi-Fi, OLSR, applications, FlowMonitor |

## Executed checks

1. The required ns-3 modules compiled successfully from the official ns-3.47 source archive.
2. A three-node/two-hop IEEE 802.11g network carried 100 UDP packets through OLSR.
3. FlowMonitor reported 100 transmitted and 100 received packets (PDR 100%) with mean delay 1.946 ms in this environment test. This is a smoke-test result, not a research result.
4. `HelloInterval` was changed at runtime from 2.0 s to 0.5 s. The OLSR Tx trace recorded nine post-change inter-HELLO intervals below 0.75 s, establishing that the proposed timer adaptation is technically executable.
5. The locked 4x4 grid geometry has an internally node-disjoint alternative path between nodes 0 and 15 and remains connected after failure of node 1.

## Runtime trace evidence

```text
G1_SMOKE ns3=3.47 run=1 nodes=3 hops=2 tx=100 rx=100 pdr_percent=100.000 mean_delay_ms=1.946
G1_OLSR_RUNTIME hello_count=13 post_change_fast_intervals=9 times=0.29631,2.38562,4.11859,6.00208,6.52393,7.09086,7.51648,8.08678,8.60295,9.09649,9.529,10.021,10.5585,
```

## Non-blocking limitations

- Optional GUI, documentation, Python bindings, GSL, and MPI dependencies were not installed because the pilot does not require them.
- The smoke test demonstrates environment correctness only. It does not validate the G2 baseline topology, workload, convergence window, or final metrics.
- Runtime changes take effect when the relevant OLSR timer next reschedules; they do not cancel an already scheduled event. G3 must account for this response lag.

## Gate boundary

No G2 baseline results have been generated. The next authorized stage is implementation and validation of fixed-parameter OLSR on the locked 16-node topology.
