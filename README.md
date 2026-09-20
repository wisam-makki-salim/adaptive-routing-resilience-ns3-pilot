# Adaptive Routing under Mobility and Failures

A reproducible ns-3 pilot that evaluates lightweight OLSR adaptation under normal operation, gradual link degradation, and node failure.

## Research question

Can a lightweight adaptive routing-control mechanism improve service resilience under changing mobility, link degradation, and failure conditions compared with a static baseline, without introducing excessive latency or routing overhead?

## Main finding

The locked evaluation does not support a general superiority claim. Adaptive service effects vary across paired seeds, while routing overhead increases whenever the controller reacts. Component ablation shows that shortening the OLSR HELLO interval reproduces the full mechanism's cost and adverse tail; event detection without timer changes reproduces the baseline exactly. A lower failure threshold is retained as an exploratory redesign candidate for independent validation.

The [four-page technical research note](Wisam_Makki_Salim_Adaptive_Routing_Pilot_Technical_Note.pdf) provides a compact account of the study.

## Repository contents

The repository includes executable simulations, fixed configurations, raw and processed results, uncertainty analysis, diagnostic traces, component ablation, figures, and a four-page technical note.

## Evidence package

- `results/g4/raw/g4_final_runs.csv`: 120 locked final-evaluation runs.
- `results/g6/raw/g6_runs.csv`: 141 robustness and ablation runs.
- `results/final_summary_table.csv`: compact summary results.
- `figures/`: PNG and PDF figures regenerated from recorded results.
- `docs/G5_ANALYSIS_REPORT.md`: paired uncertainty and adverse-tail diagnosis.
- `docs/G6_ROBUSTNESS_ABLATION_REPORT.md`: component and threshold tests.
- `docs/SUPERVISOR_FACING_SUMMARY.md`: concise outreach summary.

## Locked toolchain

- Ubuntu 24.04 LTS
- ns-3.47
- GCC 13.3.0
- CMake 4.4.3
- Ninja 1.13.2

The ns-3 source archive checksum is recorded in `environment/ns3-source.sha256`.

## G1 smoke test

The smoke test creates a three-node, two-hop IEEE 802.11g ad hoc network, installs OLSR, sends UDP traffic, and extracts delivery and delay using FlowMonitor. A second executable changes `HelloInterval` from 2.0 s to 0.5 s during simulation and verifies the change from actual OLSR transmission timestamps.

```bash
python3 -m pip install --user --break-system-packages -r environment/requirements.txt
export NS3_ROOT=/absolute/path/to/ns-allinone-3.47/ns-3.47
bash scripts/g1_build_and_smoke.sh
```

A successful run prints a line beginning with `G1_SMOKE` and exits with status 0 only when at least one packet reaches the destination.

## G2 baseline

```bash
export NS3_ROOT=/absolute/path/to/ns-allinone-3.47/ns-3.47
bash scripts/run_g2_baseline.sh
```

The script rebuilds the baseline, executes development runs 1-5, and regenerates the raw and summary CSV files. The configuration is recorded in `configs/scenario_a_baseline.yaml`; interpretation and limitations are in `docs/G2_BASELINE_REPORT.md`.

## G3 adaptive-mechanism validation

```bash
export NS3_ROOT=/absolute/path/to/ns-allinone-3.47/ns-3.47
bash scripts/run_g3_validation.sh
```

The test validates per-neighbor EWMA signal triggering, final-transmission-failure triggering, hysteresis/hold-down recovery, live OLSR timer changes, and real ns-3 Wi-Fi trace wiring. See `configs/lra_olsr.yaml` and `docs/G3_ADAPTIVE_MECHANISM_REPORT.md`.

## G4 controlled experiments

```bash
export NS3_ROOT=/absolute/path/to/ns-allinone-3.47/ns-3.47
G4_JOBS=6 bash scripts/run_g4_experiments.sh
```

This regenerates all 120 final runs and the raw, descriptive-summary, and paired-difference CSV files. The experiment is defined in `configs/g4_experiment.yaml`; the evidence boundary is documented in `docs/G4_CONTROLLED_EXPERIMENTS_REPORT.md`.

## Reproducibility note

The smoke test fixes the ns-3 seed and accepts an independent run number. Development runs are separate from the final evaluation seeds locked in G0.

## G5 paired analysis

```bash
export NS3_ROOT=/absolute/path/to/ns-allinone-3.47/ns-3.47
bash scripts/run_g5_analysis.sh
```

The script reruns only the locked C/102 diagnostic with detailed tracing, verifies exact equality with its stored G4 rows, and regenerates the paired bootstrap tables and figures. See `docs/G5_ANALYSIS_REPORT.md`. The analysis does not claim superiority: service-effect intervals cross zero in Scenarios B and C, while adaptive overhead is higher in all triggered paired runs.

## G6 robustness and ablation

```bash
export NS3_ROOT=/absolute/path/to/ns-allinone-3.47/ns-3.47
G6_JOBS=6 bash scripts/run_g6_robustness.sh
```

This executes the predeclared component ablation and threshold sensitivity matrix, regenerates paired bootstrap results, and writes two publication-ready figures. The exact design is in `configs/g6_robustness.yaml`; conclusions and evidence boundaries are in `docs/G6_ROBUSTNESS_ABLATION_REPORT.md`.

## Research boundaries

This repository demonstrates research execution rather than a state-of-the-art routing claim. Results apply to the tested topology, traffic, mobility, and failure configuration. The threshold study reuses evaluation seeds and is explicitly exploratory. Fresh seeds and a second topology are required before any redesigned controller is described as validated.

## Technical references

- [ns-3 documentation](https://www.nsnam.org/documentation/)
- [OLSR RFC 3626](https://www.rfc-editor.org/rfc/rfc3626)

## Citation and license

Citation metadata are provided in `CITATION.cff`. The repository is released under the [MIT License](LICENSE).
