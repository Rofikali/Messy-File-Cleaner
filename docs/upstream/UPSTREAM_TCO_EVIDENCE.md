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
