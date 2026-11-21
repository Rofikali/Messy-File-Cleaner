# Messy-File-Cleaner — Compact LIVE NOW / NEXT / LATER

> **Verified handoff (2026-11-21):** main `8748679d71606415883dbc2b121c46f1a00d576a`; PR #131 MERGED (tested head `ff0fb4e618c11878a561a0819ae62388cd453e96`, CMake/feature-budget, Docker, Ubuntu 24.04 and Makefile PASS). BENCH-SCALE-01 PR #132 OPEN; manual OS/Docker/Compose/real-filesystem/backup/crash NOT RUN. FS-01B P0 and REL-01 BLOCKED. This is a snapshot, not proof of current GitHub status.
>
> **One live selector:** this document; [feature budget ledger](docs/feature-budget.json) and [five-PR feature-delivery policy](docs/FEATURE_DELIVERY_AND_ACCEPTANCE_POLICY.md) enforce delivery ceilings, while [next-history.md](next-history.md) is the unchanged dated archive. Live GitHub overrides any stale snapshot.

## LIVE NOW / NEXT / LATER

| Lane | ID / budget / status | Concrete deliverable | What is NOT proven |
| --- | --- | --- | --- |
| **NOW — IMPLEMENTATION PR** | **BENCH-SCALE-01 (1/5; #132 OPEN)** | Bounded disposable Python os.scandir enumeration resource baseline; NOT engine scanner benchmark. Exact-head CI NOT RUN | No C engine million-file throughput, owner-run acceptance or release |
| **DONE** | **SCAN-ROBUST-01 (1/5; #131 merged)** | Callback-error/invalid-input preservation regressions merged at `8748679d71606415883dbc2b121c46f1a00d576a`; exact-head workflow families PASS | Manual Ubuntu/Docker/Compose/real-filesystem/backup/crash NOT RUN |
| **DONE** | **CLI-DRYRUN-01 (1/5; #130 merged)** | Read-only preview and failure status merged at `74176cf4f90984bae660960c4f59de0441f1043a`; exact-head four workflow families PASS | Manual Ubuntu/Docker/Compose/real-filesystem/backup/crash NOT RUN; no release |
| **CLOSED LEGACY** | **PERF-01 — ENGINEERING IMPLEMENTED / MERGED through #128** | Existing disposable benchmark safety, provenance, output integrity, hardlink/symlink/FIFO/Unix socket and collision regressions | Earlier feature exceeded new cap; no more PERF-01-only PRs; no end-to-end release or physical-filesystem proof |
| **P0 BLOCKED** | **FS-01B-FIX** | Security/trust/crash-model review and positive unrelated-file-preservation tests **before** an explicitly approved production correction | Known same-UID check/use unlink risk remains; a passing reproduction is NOT a fix |
| **RELEASE BLOCKED** | **REL-01** | Approved FS-01B-FIX, source/journal safety, supported filesystem/operator manual tests, backup/recovery and rollback signoff | C11/C17/C23, Docker and benchmark CI do not grant release authority |
| **DEFERRED** | **RUST-01 / ABI-01** | Only owner-approved isolated Rust testing; production .a/.so/FFI require distinct ADR/consumer and review | Current Rust is test-only |
| **LATER** | **BIZ-01** | Validate users and ownership/support liability before commercial library or paid services | No revenue, SaaS or pricing is implemented |

## 2026-11-21 completion and evidence snapshot

| Area | IMPLEMENTED / MERGED | Evidence observed | Remaining acceptance |
| --- | --- | --- | --- |
| C17 CLI, scanning/classification, queue/worker, legacy mover and journal | Existing code on main | C11/C17/C23 CMake/CTest, Makefile and Ubuntu 24.04 on main | Known source-retirement race; no approval for valuable files |
| FCJ3/FCJ4 parser/writer/testing | Isolated test-domain features, **not** production recovery | Source, tests and conditional CI lanes | No production FCJ4 link, trusted recovery or FFI |
| Disposable benchmarking and fixture refusal | PERF-01 work merged through #128 | CMake, Docker, Makefile, Ubuntu, performance workflows all success at `3d7e14ed1604` | No claimed nanosecond end-to-end latency, target-host million-file workload or power-loss test |
| Docker and developer tooling | Dev/prod Docker images, safe dry-run Compose default, guarded explicit execution, Makefile status/self-check/rust-check | Docker, Makefile and Ubuntu CI success at referenced main | Owner-run Ubuntu/Docker Desktop/Compose and external host filesystem tests NOT RUN in this audit |
| Release / real user safety | No approved release | Current repository remains prototype/development | FS-01B-FIX and REL-01 BLOCKED; no automated real file moves |

## Five-PR continuation rule (hard ceiling, not five PRs mandatory)

Each **opened feature-specific PR attempt** (merged, closed, abandoned) counts once under the same stable feature ID, including test-only refinements and renamed subtasks. The `docs/feature-budget.json` ledger is validated in required PR CI; the PR must declare `Feature-ID: BENCH-SCALE-01` and then record its actual assigned PR number in the ledger before checks can pass. A changed PR gets additional commits without consuming another slot. At five PRs (or earlier if accepted), close DONE/PARTIAL/BLOCKED with verified scope and explicit gaps and move to a different READY feature. Don't hide a sixth PR under a new milestone name or a generic security exception.

Separate *code exists*, *merged*, *CI pass at exact SHA*, *manual Ubuntu/Compose*, *real filesystem/backup/crash testing*, *security approval* and *RELEASED*. Keep unexecuted checks **NOT RUN**, NEVER "PASS". The existing separate hourly autopilot must select current READY from here and stop repeated PERF-01 test-only PRs. P0 FS-01B-FIX remains prohibited until its prerequisites are approved, independent of feature cap.

## Standing evidence and ownership rules

- Reconcile current GitHub main SHA, open PRs, merge SHAs, workflow runs and review protections **before** any change; an open PR takes precedence over creating duplicate work.
- For every material feature PR, update the ledger + this compact board + affected docs **in the same PR** and report UPDATED vs N/A; don't create repeated post-merge status-only PRs. Archive retains historical chronology; never overwrite it.
- CI is a gate, not a release certificate. Confirm exact-head checks; conditional TSan/Rust/scale work not triggered is NOT RUN.
- The owner-authorized autopilot may merge only when current-head tests, security and independent review/branch protection are satisfied, with an expected-head safeguard; never deploy, move real customer data, publish or spend.
- C17 production and C11/C23 portability remain; Rust, FCJ3/FCJ4 and API/ABI concepts retain test-only/proposed boundaries.
