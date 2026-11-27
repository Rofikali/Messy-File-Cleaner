# Upstream adoption decision gate

UPSTREAM-COST-01 final bounded milestone. Planning only; this document does not authorize source imports, external dependencies, spending, production ABI/FFI, or release.

## Decision sequence

1. Identify a concrete customer or engineering need and a measured baseline. Specify exact source revision, supported platform, workload, required safety behavior and rollback.
2. Compare retaining our C implementation, supported Linux/POSIX/libc interfaces, and a component-specific alternative. Record why each option is relevant; do not treat an unknown cost as zero.
3. Reject or defer options that lack approved license/provenance, a maintainable security boundary, ownership/ABI contract, regression tests, compatible environments or safe failure behavior. Economics cannot waive these gates.
4. Compare cash outlay and noncash owner-time separately over the same currency, minor-unit scale, horizon, and equivalent required behavior. Record source, date, units, assumptions and uncertainty of each input. Avoid double counting paid labor or owner opportunity cost. See [measured TCO evidence](UPSTREAM_TCO_EVIDENCE.md).
5. For uncertain estimates, compare plausible low/base/high scenarios, including maintenance and remediation costs. A crossover is a conditional planning result, not ROI, revenue, a guarantee of savings or approval.
6. Prefer the simplest safe supported approach with sufficient evidence. If an alternative appears cheaper but requires a gate waiver, do not adopt. If inputs are missing, record DEFER and the evidence needed. Any spending or upstream component import needs separate explicit approval.
7. Record a bounded ADR with alternatives, evidence, risks, expected maintenance owner, rollback and exact-head review; do not automatically implement a decision.

## Current decision record

| Alternative | Evidence | Decision |
|---|---|---|
| Retain existing own C code | Existing reference and production boundaries; current test evidence | RETAIN baseline; release safety blockers unchanged |
| Supported Linux/POSIX/libc APIs | Existing disposable reference probes, capability reports and safe fallback requirements | EVALUATE per concrete use case; no blanket production enablement |
| External library or copied upstream implementation | No approved registry component, no component-specific TCO or license clearance | DEFER/BLOCKED |
| Production Rust FFI or public C ABI | Private reference only; explicit scope and staged approval outstanding | BLOCKED |

Actual account billing, owner hourly rate, labor effort, customer demand, monetary savings and ROI remain UNKNOWN. No new paid service is authorized. No financial conclusion about a specific external component is supportable yet. The 39 checked arithmetic scenarios establish synthetic reference behavior, not real-project finances.

## Acceptance and next engineering selection

Validate final exact-head CI and reference tests; retain the five-PR cap. Close scoped PARTIAL if financial inputs or independent review remain missing, rather than opening a sixth PR. Do not extend this calculator into a product feature or speculative accounting system. Select the next independent engineering milestone from the live roadmap only after reconciliation. External contributions require separate explicit authority; FS-01B/REL-01, license/import and Rust FFI gates remain unchanged. Rollback: revert this documentation and test-only validation change.
