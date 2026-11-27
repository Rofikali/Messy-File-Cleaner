# Upstream reuse: measured cost baseline and decision contract

UPSTREAM-COST-01, first bounded milestone. Internal planning only; no spending, pricing, licensing or production integration authorization.

## Measured evidence

Source: PR #291 exact head 105c13e1ff521b859a47a3c361edc1506fce24b5, merged f0f95f66bc9800ba4e9fea87ef9e512f84446681. All 13 returned latest job records across four workflows passed after retrying APT HTTP 403 failures. Formal independent review not returned; TSan workflow not returned. Ubuntu workflow restoration confirmed. Elapsed time below is last minus first timestamp in the specific job log, rounded to nearest second, not billed minutes, CPU time or developer labor. Parallel jobs must not be summed as wall-clock latency. Selected observations only, not all CI costs.

| Job | Job ID | Observed log span (seconds) | Full-suite CTest wall time (seconds) |
|---|---:|---:|---:|
| Standard GCC C17 Release | 140302203044 | 39 | 19.29 |
| Standard GCC C17 Debug | 140302203132 | 34 | 18.70 |
| Ubuntu C23 retry | 140303430639 | 70 | 16.84 |
| Makefile retry | 140303420197 | 133 | Four suites: 17.24, 17.09, 16.80, 17.04 |
| Ubuntu image and Clang validation | 140303431369 | 126 | Container Clang Release 18.55 |
| Failed Makefile package fetch | 140302203021 | 15 | Not run |
| Failed Ubuntu C23 package fetch | 140302243949 | 17 | Not run |

The two failed attempts account for about 32 seconds of observed log spans; successful retries account for about 203 seconds, excluding queues and other work. No invoice, quota, usage dashboard, energy or human-time evidence supplied; monetary cost UNKNOWN, not zero. The reporter's deliberate ten-second timeout test in each full suite is a meaningful measurable validation cost; do not remove it solely to improve timing. Measure repeated matched samples before claiming a regression or savings. Source [CI logs](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/45936941532); [standard matrix](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/45936941395); [Makefile](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/45936941518).

## Options and gates

| Alternative | Integration/maintenance exposure | Decision |
|---|---|---|
| Retain own C implementation | Own defect fixing and test maintenance | Current baseline |
| Use supported POSIX/Linux/libc interfaces | Platform capability, compatibility, fallback testing | Prefer when evidenced benefit |
| Approved maintained external library | Dependency, supply chain, patching, licensing and ABI obligations | Component-specific proposal only |
| Copy upstream implementation | Provenance, local fork divergence, security tracking and legal burden | Blocked until explicit license and component review |

Registry empty and locked; no component-specific imported-code financial comparison possible. Production FFI and SDK are blocked. Real consumer demand and safety are gates, not financial variables that may be traded away.

## Comparable-horizon TCO contract

Use same currency and time horizon for each option, and document whether each amount is measured, quoted, estimated or unknown. Record integration engineering hours, validation/review hours, recurring maintenance/support hours, CI usage, licensing/compliance, distribution/operations, expected remediation and opportunity cost. Convert labor with a stated hourly rate, CI with actual provider billing terms and included quota, and cash items separately. Avoid counting the same developer hours twice as direct labor and opportunity cost. Keep noncash owner time separate from cash outlay. Unknown values are not zero.

For horizon H months: TCO = one-time integration + validation + compliance + sum of monthly recurring maintenance, CI, operations and incremental risk-remediation scenarios. Compare alternatives using equal required behavior, scope, safety and time horizon. Quantify net savings = baseline TCO minus candidate TCO only when both have sufficient grounded inputs; otherwise show scenarios and uncertainty. Risk-related estimates are not guaranteed losses or security certificates. No revenue or willingness-to-pay evidence; no ROI or payback claim. No new paid services.

## Next executable acceptance

If justified, implement a dependency-free, test-only calculator with bounded integer units, explicit unknown/invalid rejection, same-horizon comparisons and synthetic controls. No automatic purchases or workflow/provider changes. Measure actual repeated CI samples and use owner-supplied rates only for financial estimates, not inferred bills. Record if maintenance/reuse alternatives lack sufficient evidence; close scoped DONE/PARTIAL/BLOCKED within five opened PRs, no budget reset. No FS-01B/REL-01, license, production ABI/FFI or release gate changes. Rollback: revert this reference-only documentation.


## UPSTREAM-COST-01 — checked integer comparison

#292 merged at `cbf8b068564cb493213cfa55fef8ac9a262b07c5`, tested head `29ac7703fb87cc720fd99e11d59beecc710074be`; 13/13 returned exact-head jobs PASS (CMake 45937918563, Ubuntu 45937918525, Docker 45937918686, Makefile 45937918541); formal independent review and TSan workflow not returned. #293 adds a [BUILD_TESTING-only reference](../../tests/reference/upstream_tco_reference.c), using unsigned 64-bit minor-unit amounts, three one-time and four constant monthly cost categories, matching three-uppercase-letter currency identifiers and horizons of 1–120 months, explicit known/unknown flags, checked addition and horizon multiplication, and unsigned difference plus direction. Reject unknown amounts, invalid metadata, null arguments and arithmetic overflow; do not publish a result on failure. The currency identifier is syntactic, not an ISO currency registry lookup; amounts must use the same minor-unit scale. Seventeen synthetic checks include equal/higher/lower options and error controls. Categorical values and horizons are illustrative, not project financial measurements; no labor-rate conversion, tax, discounting, inflation, varying monthly costs, billing ingestion, revenue or ROI model. Local NOT RUN; exact-head CI pending. No dependency, production linkage, import, spend or release; Rust FFI and FS-01B/REL-01 remain blocked. Rollback: revert PR.


## UPSTREAM-COST-01 — bounded horizon sensitivity

#293 merged at `f7ae637aea60eb831dede7fd89b0a809bc551d1e`, tested head `c22f4457091c655006d277814cf6b1490bb35220`; 14/14 returned jobs PASS (CMake 45938409448, Ubuntu 45938409320, Docker 45938409369, Makefile 45938409298, FCJ4 TSan 45938409307); GCC C17 Release 82/82 (job 140306836990), Debug 78/78 (job 140306836973), TCO reference CTest passed. Formal independent review not returned. #294 extends the [private reference](../../tests/reference/upstream_tco_reference.c) with a bounded 1–120 month sweep using the checked comparison per horizon, lower/equal/higher counts and first strictly lower horizon (zero when none). Output remains unchanged on failed evaluation, including later overflow. Twelve additional synthetic checks cover equal alternatives, crossover at month 12 with first strictly lower month 13, reversed comparison, truncated horizon, nonintegral crossover, immediate savings, late overflow, unknown and invalid arguments. Fixed monthly costs, no extrapolation, discounting, inflation, changing costs or financial recommendation. Existing 17 checks retained; local NOT RUN, exact-head CI pending. No actual charges, ROI, dependencies, imports, production changes or spend. Licensing, Rust FFI and FS-01B/REL-01 remain blocked. Rollback: revert PR.


## UPSTREAM-COST-01 — cash and noncash economic cost separation

#294 merged at `f77a47765a2907757aab74d209a5cb0a374e860f`, tested head `71000f1d27d8b7f62354ed62b5d112534ce3b260`; 13/13 returned jobs PASS (CMake 45939596896, Ubuntu 45939596771, Docker 45939596841, Makefile 45939596844); GCC C17 Release 82/82 (job 140310606069), Debug 78/78 (job 140310606118), existing TCO CTest with 29 synthetic checks passed. Formal independent review and TSan workflow not returned. #295 extends the [private reference](../../tests/reference/upstream_tco_reference.c) with separate cash and noncash owner-time option records, same currency and horizon, checked combined total and atomic failure. Cash can be evaluated independently even when noncash inputs remain unknown. Caller must allocate each economic amount once; the reference does not detect overlapping categories or convert hours to rates. Ten additional synthetic checks cover zero cash/nonzero noncash, independently known cash, unknown amounts, metadata mismatch, combined overflow and null arguments. Fixed monthly assumptions; not a financial accounting, billing or profitability system. Local NOT RUN; exact-head CI pending. No actual charges, ROI, dependencies, imports, production changes or spending. Licensing, Rust FFI and FS-01B/REL-01 remain blocked. Rollback: revert PR.
