# Pilot Study v2 Independent Scientific Review

## Overall judgment

The v2 package is suitable as evidence of research execution for initial PhD
supervisor contact. It is not ready to support a publication claim that the tested
state-triggered controller improves resilience. Its value lies in the fair
three-way comparison, the discovery and correction of a mechanism-dependent
disturbance confound, and the retention of negative results.

## Critical issues

No unresolved issue invalidates the reported v2 B0/B1/B2 comparisons after the
disturbance target was fixed across mechanisms. Earlier recovery-time analyses
that treated -2 as a number would be invalid; v2 corrects this by reporting
non-recovery separately.

## Major issues

1. **B2 fails the stable negative control.** Three stable runs triggered. The
   20-event threshold cannot be presented as a safe or validated controller.
2. **The model is narrow.** One topology family, deterministic log-distance loss,
   two UDP flows, and one OLSR implementation limit external validity.
3. **Always-on gains have a high cost.** B1 improves delivery in B–D but increases
   normalized overhead by about 0.05 and raises p95 delay in most paired runs.
4. **Recovery remains an aggregate metric.** In D it can equal zero even after a
   disruptive interval because measurement begins at the final return boundary.
   Minimum-flow PDR is therefore a necessary guardrail.
5. **The controller rarely reverts.** B2 recorded no recovery transitions in A–C
   and seven in D. The hold-down logic depends on renewed signal evidence from the
   trigger neighbor and may leave nodes reactive after route changes.
6. **The literature position is preliminary.** The four verified sources define
   the immediate boundary, but a journal submission would require a systematic,
   current review of adaptive OLSR, event-triggered networking, and safe control.

## Minor issues

- The executable configuration is still duplicated across YAML, C++, and shell.
- The repository needs CI for compilation, smoke execution, and analysis checks.
- The PDF build should be deterministic or document expected metadata variation.
- Scenario D should eventually use a per-event service trace rather than one
  post-final-event recovery value.

## Originality assessment

Changing HELLO or TC intervals is established prior work. The credible originality
path is not timer adaptation alone. It is the design and evaluation of a guarded
trigger and exit policy that explicitly constrains false activation, cross-flow
harm, control occupancy, and tail risk across several topology families.

## Publication boundary

The present evidence can support a workshop-style negative-results or methodology
paper only after broader validation and a stronger literature review. For supervisor
outreach it is already useful because the repository shows implemented code, fresh
experiments, a corrected confound, reproducible analysis, and disciplined limits.
