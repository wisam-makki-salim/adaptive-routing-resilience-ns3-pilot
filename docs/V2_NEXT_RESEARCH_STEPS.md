# Pilot Study v2 Next Research Steps

## Guarded trigger and exit policy

**Motivation.** B1 demonstrates available delivery benefit, while B2 misses most
of it and triggers falsely in stable runs.

**Question.** Can route-state and congestion safeguards decide when fast HELLO
control is useful and when it should stop?

**Method.** Add a finite reactive budget, route-change confirmation, channel-load
guard, and explicit exit timeout. Pre-register thresholds on development seeds,
then evaluate once on fresh seeds.

**Difficulty.** The safeguards may suppress useful reactions or simply shift the
threshold problem. State observability differs between simulation and real systems.

**External PhD feasibility.** High. The work is simulation-based, modular, and can
be conducted from Iraq with short visits for supervisory review or testbed checks.

## Multi-topology and mobility validation

**Motivation.** Current results come from one jittered grid and one selected relay.

**Question.** Which benefit-cost patterns persist across density, mobility, fading,
traffic mix, and failure location?

**Method.** Use a factorial but bounded design with random geometric and corridor
topologies, selected mobility models, heterogeneous flows, and blocked evaluation
seeds. Model topology as a random effect where the sample supports it.

**Difficulty.** The design space grows quickly. A staged protocol and compute
budget are required to avoid indiscriminate parameter sweeps.

**External PhD feasibility.** High if simulations run on reproducible remote
compute. Annual visits can focus on design review and selected physical validation.

## Safety constraints for adaptive network control

**Motivation.** Aggregate improvement can coexist with cross-flow harm, false
activation, or excessive control occupancy.

**Question.** Can adaptation be selected under explicit constraints on worst-flow
delivery, tail loss, delay, and control cost?

**Method.** Formulate constrained or multi-objective policy selection using only
validation data. Evaluate Pareto stability on unseen topologies and report policy
violations rather than one accuracy-like score.

**Difficulty.** Constraints may conflict, and a Pareto frontier can be unstable
with few environments. The work must avoid turning post-hoc tuning into a claimed
general policy.

**External PhD feasibility.** Moderate to high. Most work is computational, but a
small testbed collaboration in Germany would materially strengthen the thesis.
