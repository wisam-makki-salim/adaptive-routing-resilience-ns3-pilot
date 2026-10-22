# Pilot Study v2 Supervisor Readiness Assessment

## Rubric

| Dimension | Score | Evidence | Main limitation |
|---|---:|---|---|
| Research question clarity | 4.5/5 | One three-way question, two bounded secondary questions, and pre-specified hypotheses | “When” is tested in four controlled conditions, not generalized contexts |
| Technical implementation | 4.0/5 | ns-3.47 code, B0/B1/B2, repeated disturbance, fixed targets, 360 fresh runs | No controller unit test or CI |
| Experimental validity | 3.8/5 | Paired seeds, exogenous disturbance target, retained runs, guardrail metrics | One topology and channel family; recovery metric remains coarse |
| Reproducibility | 4.2/5 | Raw data, logs, configuration, scripts, deterministic analysis, generated figures | ns-3 installation is not yet automated in the repository |
| Originality potential | 3.5/5 | Safety-constrained triggering and exit control emerge from observed failure | Timer adaptation itself is established work |
| Supervisor relevance | 4.3/5 | Direct fit to adaptive networking, resilience, dependable control, and simulation | No supervisor-specific paragraph in the generic package |
| Doctoral extension feasibility | 4.1/5 | Clear next experiments and bounded engineering path | Multi-topology validation will require sustained compute and careful scope |

The mean score is 4.06/5. This is an evidence-based readiness score for supervisor
outreach, not a publication-quality rating. The package demonstrates independent
execution and critical judgment. It should be presented as preliminary work whose
main controller hypothesis was not confirmed.

## Outreach decision

**Ready for selective supervisor outreach after the repository branch and final
documents are published.** The first message should lead with the problem and the
negative but informative result: permanent fast control improved delivery at high
cost, while the tested trigger failed to capture that benefit reliably and produced
false activations. This leaves a specific doctoral question rather than pretending
the pilot has solved it.
