# Pilot Study v2 Independent Reproduction Record

Date: 22 October 2026

## Scope

This check rebuilt ns-3.47 in a new temporary directory without reusing the
repository's existing `build` or `cmake-cache` directories. It then executed the
complete v2 pipeline: 30 disturbance-target probes followed by 360 simulations
covering four conditions, three mechanisms, and runs 201–230.

## Environment

- Operating system: Ubuntu 24.04
- Compiler: GCC 13.3.0
- CMake: 4.4.3
- Ninja: 1.13.2
- ns-3: 3.47, optimized profile
- Parallel simulation jobs: 8
- ns-3 source archive SHA-256: `577da01df6386b57ce362b7b82b8d3bdab7f748bc0836ba51249c8dae743ff2e`
- Source commit tested before the corrective commit: `132019ce0c9f0ab2cf3f6c338372a1a298775f47`

Optional ns-3 components unavailable in the clean environment were reported by
the configure step and are not dependencies of this experiment. The required
core, network, mobility, propagation, Wi-Fi, internet, applications, OLSR, and
FlowMonitor modules built successfully.

## Result

- Expected result rows: 360
- Unique result keys: 360
- Missing result keys: 0
- Validation status: PASS
- Full-pipeline elapsed time after the clean build: 510 seconds
- Regenerated raw CSV SHA-256: `3eec18216e617a081a9cc550167e0a0f39259c5feaedf3cafd9f14bd9c824de8`
- Previously committed raw CSV SHA-256: `3eec18216e617a081a9cc550167e0a0f39259c5feaedf3cafd9f14bd9c824de8`
- Byte-for-byte raw CSV comparison: PASS

The full independent rerun reproduced the committed raw data exactly. This is a
deterministic reproduction of the implemented simulation under the recorded
toolchain. It does not extend the study's external-validity boundary beyond the
tested topology, traffic, propagation, and disturbance design.

## Confidence-interval correction

The earlier analysis used one stateful bootstrap random-number generator across
several output stages. Repeating the same estimand later in the script therefore
used a different Monte Carlo sample, which could produce small differences among
table and figure intervals. The raw data and point estimates were not affected.

The corrected analysis derives a fixed random seed from the name of each
scenario, comparison, and metric. It writes the resulting values once to
`results/v2/v2_canonical_summary.json`. CSV summaries, figures, and supervisor
documents now consume that same canonical record. Cross-table interval comparison
reported zero mismatches after regeneration.
