# Pilot Study v2 — Locked Research Protocol

## Working title

**When Should Adaptive Network Recovery Be Triggered? A Reproducible Study of
Resilience and Control-Cost Trade-offs in Mobile IoT Networks**

## Literature boundary

Timer adaptation is not itself a novelty claim. OLSR defines periodic HELLO and TC
control messages, with default intervals of 2 s and 5 s respectively. Prior work
has already examined adaptive MANET routing, context-dependent switching, and
adaptive HELLO messaging. The unresolved question addressed here is narrower:
whether state-triggered use of faster OLSR control provides a better conditional
benefit-cost profile than both static control and permanent fast control under the
same disturbances.

Relevant verified sources include:

1. T. Clausen and P. Jacquet, “Optimized Link State Routing Protocol (OLSR),”
   RFC 3626, 2003. DOI: `10.17487/RFC3626`.
2. L. Qin and T. Kunz, “Adaptive MANET Routing: A Case Study,” in *ADHOC-NOW
   2008*, pp. 43–57. DOI: `10.1007/978-3-540-85209-4_4`.
3. J. Hoebeke et al., “Adaptive routing for mobile ad hoc networks,” *EURASIP
   Journal on Wireless Communications and Networking*, 2012:126.
   DOI: `10.1186/1687-1499-2012-126`.
4. S. Y. Han and D. Lee, “An Adaptive Hello Messaging Scheme for Neighbor
   Discovery in On-Demand MANET Routing Protocols,” *IEEE Communications
   Letters*, 17(5), 1040–1043, 2013.
   DOI: `10.1109/LCOMM.2013.040213.130076`.

These sources establish that adaptive routing and HELLO control are existing
research directions. They do not answer the three-way comparison implemented in
this pilot under its particular failure and repeated-disturbance design.

## Primary research question

Under stable, progressive, abrupt, and repeated network disturbances, when does
state-triggered OLSR timer adaptation improve packet-delivery resilience relative
to static OLSR and always-on fast control, and what control-overhead and harmful-tail
costs accompany any improvement?

## Secondary questions

1. Does state-triggered control avoid the stable-condition cost of always-on fast
   control without reproducing the adverse tail observed in the earlier pilot?
2. Are any benefits consistent at the worst-flow level and after repeated
   disturbances, rather than visible only in aggregate delivery?

## Hypotheses

- **H1:** In the stable negative control, state-triggered operation remains equal
  to static OLSR within deterministic simulation output, while always-on fast
  control increases normalized OLSR overhead.
- **H2:** Under abrupt and repeated disturbances, state-triggered operation with
  the pre-specified 20-event failure threshold has a more favorable paired PDR
  versus overhead profile than always-on fast control.
- **H3:** Any aggregate PDR improvement is not considered operationally credible
  if minimum-flow PDR deteriorates or harmful-tail frequency increases.

## Contribution statement

The pilot contributes a reproducible three-way mechanism comparison and an honest
failure-aware evaluation protocol. It does not claim a new routing protocol,
state-of-the-art performance, or generality beyond the tested model.

## Experimental design

- Simulator: ns-3.47.
- Topology: the existing 16-node jittered 4×4 model, retained to isolate the
  comparison change.
- Traffic: the existing two paired UDP flows.
- B0: static OLSR, HELLO 2.0 s and TC 5.0 s.
- B1: always-on fast control, HELLO 0.5 s and TC 1.0 s.
- B2: state-triggered control, -81 dBm signal threshold and a pre-specified
  20-final-failure threshold, with HELLO 0.5 s and TC 1.0 s while reactive.
- Conditions: A stable; B progressive one-way mobility; C abrupt active-next-hop
  radio failure; D two out-and-back mobility disturbances.
- Fresh paired runs: 201–230 for every condition and mechanism (360 runs).
- For each run, the disturbed node is first identified from the static B0 route and
  then fixed across B0, B1, and B2. Mechanism-dependent target selection is prohibited.
- Primary estimand: paired PDR difference relative to B0.
- Guardrails: minimum-flow PDR, normalized OLSR overhead, recovery time, p95 delay,
  route-unavailable samples, and the direction and extremes of paired effects.

## Replication and inference

The earlier Scenario B paired PDR standard deviation was about 0.52 percentage
points, whereas Scenario C was about 4.43 percentage points because of the adverse
tail. Thirty fresh pairs are computationally feasible and provide an independent
confirmation set, but they cannot support a narrow superiority claim in the
high-variance failure condition. The analysis therefore reports paired effect
distributions, deterministic 50,000-resample percentile-bootstrap intervals, and
harm direction counts. It makes no binary significance claim and performs no
post-outcome threshold selection.

## Decision rule

State-triggered adaptation is considered promising only for conditions where it:

1. has a positive central PDR effect with uncertainty reported;
2. does not create a material minimum-flow PDR tail;
3. costs less normalized overhead than always-on control; and
4. does not rely on excluding unfavorable runs.

Failure of any rule is reported as a limitation or negative result, not hidden.

## Gate B/C decision

The research question is coherent, testable, and compatible with the repository.
The approved expansion is limited to the B0/B1/B2 comparison, Scenario D, fresh
paired runs, validated logging, and analysis figures. Broader topology, fading,
energy, adversarial, or protocol comparisons remain outside this execution gate.
