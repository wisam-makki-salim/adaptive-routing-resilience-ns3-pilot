# Pilot Study v2 — Independent Repository Audit

Audit basis: repository `wisam-makki-salim/adaptive-routing-resilience-ns3-pilot`,
main commit `e26647b`, inspected on 22 October 2026.

## Gate A decision

**Conditional pass for research redesign; experimental expansion was blocked until
the comparison design and execution checks below were specified.** The stored G4
and G6 tables are internally complete, their keys are unique, and every committed
CSV analysis table was reproduced exactly from the committed raw data. Regenerated
PNG figures were byte-identical. PDF figure hashes differed because PDF metadata
are not deterministic; the underlying plotted data come from the same scripts.

The audit did not independently reproduce the ns-3 simulations because ns-3.47 was
not present in the audit environment. Stored simulation outputs are therefore
verified as internally consistent, not independently re-executed at this gate.

## Findings

| Finding | Evidence | Severity | Scientific impact | Correction |
|---|---|---:|---|---|
| The comparison contains only static OLSR and state-triggered adaptation. It cannot distinguish the value of triggering from the value or harm of permanently faster control. | `configs/g4_experiment.yaml:6`; `scripts/run_g4_experiments.sh:22-27` | Major | The core causal comparison required for “when to trigger” is absent. | Add B1 always-on adaptation and retain B0 static and B2 state-triggered under paired seeds. |
| The three original scenarios include stable operation, one progressive disturbance, and one abrupt failure, but no repeated or fluctuating disturbance. | `configs/g4_experiment.yaml:7-16`; `src/g4_controlled_experiment.cc:414-421` at the audited commit | Major | Trigger stability and repeated recovery cannot be assessed. | Add a pre-specified repeated out-and-back mobility condition without changing the original A–C scenarios. |
| The main experiment configuration is duplicated between YAML, C++, and shell scripts rather than loaded from one executable configuration source. | `configs/g4_experiment.yaml`; `src/g4_controlled_experiment.cc:250-425`; `scripts/run_g4_experiments.sh:18-49` | Major | Configuration drift can silently break provenance. | Treat the new YAML as a locked protocol and add validation of the full task matrix and CSV schema; a later engineering step should generate commands from a machine-readable manifest. |
| The execution scripts suppress simulator standard error and retain only the last output line. | `scripts/run_g4_experiments.sh:31-36`; `scripts/run_g6_robustness.sh:35-41` | Major | A failed or partially failed run can be hard to diagnose. | Preserve one log per run, stop on simulator failure, and validate the 22-field output row before aggregation. |
| The repository pins analysis packages and records an ns-3 archive checksum, but it does not fetch/build ns-3 or run the simulation pipeline in CI. | `environment/requirements.txt`; `environment/ns3-source.sha256`; absence of a workflow file | Major | A new reviewer cannot reproduce simulation results with one documented command. | Add an installation/build script or container and CI smoke test after the v2 implementation is validated locally. |
| Recovery is calculated from aggregate received packets and can hide one-flow failure. | `src/g4_controlled_experiment.cc:218-245`; `docs/G5_ANALYSIS_REPORT.md` adverse-tail discussion | Major | A “recovered” network may still be unfair or unusable for one flow. | Keep recovery as a secondary measure and report minimum-flow PDR and harmful-tail frequency as guardrails. |
| Scenario B defines recovery after movement ends at 75 s, although degradation begins at 50 s. | `src/g4_controlled_experiment.cc:225-243`; `src/g4_controlled_experiment.cc:181-203` | Moderate | The metric measures post-displacement recovery, not disruption accumulated during movement. | State this operational definition explicitly and analyze PDR and minimum-flow PDR over the full application window. |
| The topology is a jittered grid with deterministic log-distance loss, two constant-rate flows, and one chosen next-hop disturbance. | `src/g4_controlled_experiment.cc:281-425` | Major | Results do not establish generality to mobile IoT deployments. | Keep this as a controlled mechanism study; reserve additional topologies, mobility families, fading, and traffic heterogeneity for doctoral extension or a later confirmation tier. |
| The G7 report lists a DOCX that is not present in the repository. | `docs/G7_RESEARCH_PACKAGE_REPORT.md:10`; repository file inventory | Moderate | The package description is factually inconsistent. | Remove the absent-file claim or add the validated source document in the final v2 package. |
| The technical note cites protocol and research-practice sources but does not position the question against peer-reviewed adaptive-routing work. | `docs/TECHNICAL_NOTE.md:62-66` | Major | The research gap and contribution boundary are under-supported. | Add verified peer-reviewed related work and avoid claiming novelty from timer adaptation itself. |
| No automated unit test exercises controller state transitions or recovery cancellation. | `src/lra_olsr_controller.h:63-248`; no test directory at the audited commit | Moderate | Regression risk remains in state-machine changes. | Add deterministic controller tests or a small ns-3 validation scenario before merging v2. |
| Fixed streams are assigned to position and application-start jitter, and paired disruption targets match across all stored baseline/adaptive pairs. | `src/g4_controlled_experiment.cc:279-305,390-410`; `results/g4/g4_qc.txt` | Minor / positive control | Supports paired fairness in the existing G4 evidence. | Retain seed pairing and verify disruption-target equality in v2. |
| Stored G4 and G6 outputs contain the expected 120 and 141 unique rows with no missing cells; committed analysis tables reproduce exactly. | `results/g4/g4_qc.txt`; `results/g6/g6_validation.txt`; independent audit rerun | Minor / positive control | Supports internal reproducibility of analysis from stored outputs. | Retain and extend the same structural checks for v2. |

## Implemented, observed, inferred, and proposed

- **Implemented at the audited commit:** static and triggered OLSR, three controlled
  conditions, paired runs 101–120, component ablation, threshold sensitivity,
  deterministic analysis, raw tables, traces, and figures.
- **Observed in stored evidence:** no cost in the stable negative control; increased
  overhead after triggering; unstable service effects; an adverse Scenario C tail;
  exact detection-only equality with baseline; HELLO-only equality with the full
  mechanism in the reported network fields.
- **Inferred cautiously:** aggressive HELLO acceleration is the implemented action
  responsible for the measured cost and adverse tail in this topology family. The
  lower-level mechanism is not identified.
- **Proposed for v2:** fresh paired B0/B1/B2 evaluation, a repeated-disturbance
  condition, explicit guardrails, and a logged, schema-validated execution path.

## Gate boundary

Gate A permits Phase B and the implementation of the approved minimal comparison
upgrade. It does not validate new results until ns-3.47 execution is completed.
