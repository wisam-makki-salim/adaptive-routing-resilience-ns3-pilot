# Adaptive Routing under Mobility and Failures

## A Reproducible ns-3 Pilot with Robustness and Component Ablation

**Wisam Makki Salim**  
Computer Engineering Researcher  
Al Iraqia University Baghdad Iraq  
ORCID 0009-0000-6998-3912

## Abstract

This study tests whether a lightweight adaptive routing-control mechanism can improve service resilience when mobility, link degradation, or node failure changes network conditions. An ns-3.47 experiment compares standard OLSR with a controller that monitors received signal strength and final transmission failures, then temporarily shortens OLSR HELLO and topology-control intervals. The evaluation uses a 16-node wireless ad hoc topology, two concurrent UDP flows, three controlled scenarios, and 20 paired runs per comparison. The results do not support the original superiority hypothesis. Delivery and recovery effects vary across runs, while normalized routing overhead increases whenever adaptation triggers. Component ablation shows that HELLO acceleration, rather than event detection or topology-control acceleration, reproduces both the overhead and an adverse cross-flow tail event. A lower failure threshold produces exploratory gains but has not been tested on independent runs or another topology.

## Research Question and Hypothesis

The research question is: Can a lightweight adaptive routing-control mechanism improve service resilience under changing mobility, link degradation, and failure conditions compared with a static baseline, without introducing excessive latency or routing overhead?

The pre-specified working hypothesis was that event-triggered OLSR timer adaptation would reduce disruption and recovery time while keeping delay and control overhead within a modest range. The experiment was designed to test this claim rather than assume that faster routing control is beneficial.

## Experimental Design

The implementation uses ns-3.47 and IEEE 802.11g in ad hoc mode. Sixteen nodes form a jittered four-by-four grid. Two concurrent UDP flows cross the topology, each transmitting 512-byte packets every 20 ms from approximately 20 s to 110 s of a 120 s simulation. OLSR uses a 2 s HELLO interval and a 5 s topology-control interval in the baseline.

The adaptive controller maintains per-neighbor signal estimates using an exponentially weighted moving average. It enters a reactive state after three weak-signal samples at or below -81 dBm or after 40 final transmission failures within a one-second burst. The original reactive state changes HELLO from 2.0 s to 0.5 s and topology control from 5.0 s to 1.0 s. Hysteresis and a ten-second hold-down govern reversion.

Three scenarios were locked before final evaluation. Scenario A is normal operation and serves as a negative control. Scenario B gradually moves the active next-hop node away from its original location between 50 s and 75 s. Scenario C disables the active next-hop radio at 60 s. For each scenario and mechanism, runs 101 through 120 use the same ns-3 seed pairing. The primary measures are packet delivery ratio, worst-flow delivery ratio, end-to-end delay, goodput, normalized OLSR overhead, recovery time, route changes, and route-unavailable samples.

Uncertainty is calculated from adaptive-minus-baseline paired differences using 50,000 deterministic percentile-bootstrap resamples. The analysis reports means, medians, intervals, and the number of pairs showing lower, equal, or higher values. Runs are retained regardless of direction or magnitude.

## Locked Evaluation Results

Scenario A produces identical baseline and adaptive outputs in all 20 pairs, and no trigger fires. This negative control confirms that the controller adds no cost while it remains stable.

In Scenario B, the mean paired PDR difference is +0.057 percentage points with a 95 percent bootstrap interval from -0.119 to +0.312. Adaptive PDR is lower in 10 pairs, equal in 6, and higher in 4. Normalized overhead increases in every pair, with a mean difference of +0.002518 and an interval wholly above zero. The mechanism therefore incurs a repeatable control cost without a repeatable resilience benefit under gradual degradation.

In Scenario C, the median PDR effect is zero and the mean paired difference is -0.659 percentage points, with a 95 percent interval from -2.777 to +0.604. Nine pairs improve, four are equal, and seven decline. The mean is strongly influenced by run 102, where PDR falls from 95.51 percent to 76.34 percent and recovery increases from 7 s to 36 s. The run was reproduced exactly with additional temporal and per-flow traces, so it was retained as evidence of tail risk.

## Component Ablation and Sensitivity

The Scenario C controller was separated into detection-only, HELLO-only, and topology-control-only variants. Detection-only matches the baseline exactly across every network field in all 20 paired runs. HELLO-only matches the full adaptive mechanism exactly. Topology-control-only is nearly identical to baseline, with one improved pair and no stable overhead increase. These checks isolate shortening the HELLO interval as the cause of the full mechanism's cost and adverse tail in the tested configuration.

The run 102 diagnostic supports the same conclusion. Disabling the trigger, recording the trigger without changing timers, or changing only the topology-control interval all produce the baseline result. Changing only HELLO reproduces the full PDR loss and 36 s recovery time. Per-flow traces show that one flow resumes while another remains largely undelivered despite a source route, indicating a network-wide downstream interaction. The available traces do not distinguish routing-state inconsistency from wireless contention, so the lower-level cause remains open.

Threshold sensitivity also shows why the controller should not be described through one tuned operating point. A failure threshold of 20 events yields a mean PDR gain of +0.434 percentage points with an interval from +0.082 to +0.884 and a mean recovery reduction of 0.45 s. Its leave-one-out mean remains positive, but overhead increases in every run. This setting is an exploratory redesign candidate, not a confirmed result, because it was evaluated after the original experiment on the same topology family. A threshold of 80 events never triggers and simply reproduces baseline behavior.

In Scenario B, a -78 dBm signal threshold produces 317 signal transitions across 20 runs and a large overhead increase. At -84 dBm, signal triggering disappears and the failure channel triggers in 19 runs. The two detection channels therefore substitute for one another, preventing a simple claim that one RSSI threshold is optimal.

## Limitations

The experiment uses one topology family, one node density, two constant-rate flows, and an idealized node failure. The adaptive action changes protocol timers but does not explicitly measure channel occupancy or detect route loops. Recovery is defined from aggregate application reception, which can conceal flow-level unfairness without the diagnostic traces. Bootstrap intervals quantify variability among the 20 locked pairs but do not establish generality across environments. The sensitivity study reuses the evaluation seeds and must remain exploratory.

## Doctoral Research Direction

The next research step is a guarded adaptive-control design rather than a more aggressive timer change. A doctoral extension would combine link-risk evidence with route-state and congestion safeguards, cap the duration and scope of HELLO acceleration, and test whether the controller can avoid cross-flow harm. Evaluation should use fresh seeds, multiple topology and mobility families, heterogeneous traffic, and explicit tail-risk and fairness measures. Adversarial disruption can then be added as one controlled factor instead of being mixed prematurely with mobility and congestion.

This direction follows directly from the pilot's strongest result: adaptation needs a safety policy that accounts for network-wide effects. The executable repository provides a baseline, failure case, measurement pipeline, and ablation framework for that work.

## Conclusion

The original timer-switching mechanism does not provide robust evidence of improved resilience. It raises control overhead and can create severe cross-flow harm after failure. Component ablation identifies HELLO acceleration as the responsible action, while detection alone is neutral. The resulting code, controlled comparisons, uncertainty analysis, and diagnostic traces provide a reproducible basis for studying safer adaptive control.

## References

1. The ns-3 Consortium. ns-3 Network Simulator Documentation. https://www.nsnam.org/documentation/
2. Clausen T and Jacquet P. Optimized Link State Routing Protocol OLSR. RFC 3626. Internet Engineering Task Force 2003. https://www.rfc-editor.org/rfc/rfc3626
3. German Research Foundation. Guidelines for Safeguarding Good Research Practice. https://www.dfg.de/en/research-funding/funding-principles/good-scientific-practice
