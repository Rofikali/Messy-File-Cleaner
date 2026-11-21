# Messy-File-Cleaner — Compact LIVE NOW / NEXT / LATER

> **Verified handoff (2026-11-22):** main `ebf757cc0d66c78aa80b105f423535c426a435cf`; CLI-ROOT-01 #136 MERGED, tested head `9f9f650eec40c1ed43dc3eb48e5890939e955c02`, exact-head CMake/feature-budget/Docker/Ubuntu 24.04/Makefile/FCJ4 TSan PASS. CLI-CONFIG-01 PR #137 OPEN is current implementation PR: reject repeated --config before any config access. Local and exact-head CI NOT RUN; manual OS/Docker/Compose/real-filesystem/backup/crash NOT RUN; FS-01B P0 and REL-01 BLOCKED.
>
> **One live selector:** this document; [feature budget ledger](docs/feature-budget.json) and [five-PR feature-delivery policy](docs/FEATURE_DELIVERY_AND_ACCEPTANCE_POLICY.md) enforce delivery ceilings, while [next-history.md](next-history.md) is the unchanged dated archive. Live GitHub overrides any stale snapshot.

## LIVE NOW / NEXT / LATER

| Lane | ID / budget / status | Concrete deliverable | What is NOT proven |
| --- | --- | --- | --- |
| **NOW — IMPLEMENTATION PR** | **CLI-CONFIG-01 (1/5; PR #137 OPEN)** | Reject repeated `--config` selectors before configuration/filesystem access; negative CTest | Exact-head CI pending; no source-retirement or release approval |
| **DONE** | **CLI-ROOT-01 (1/5; #136 merged)** | Duplicate roots rejected at `ebf757cc0d66c78aa80b105f423535c426a435cf`; tested head `9f9f650eec40c1ed43dc3eb48e5890939e955c02` | Manual Ubuntu/Docker/Compose/real-filesystem/backup/crash NOT RUN |
| **DONE** | **CLI-MODE-01 (1/5; #135 merged)** | Conflicting `--once`/`--watch` rejected at `d6ea61cb9d6d4ffbd034ab1c3d7f27bdc20815e4`; tested head `93e8a3ccea412bf3ac5e46f9ce826de91ba14e1f` | Manual Ubuntu/Docker/Compose/real-filesystem/backup/crash NOT RUN |
| **DONE** | **CLI-INPUT-01 (1/5; #134 merged)** | Strict `--workers` parsing merged at `18b467f40d5f4be853b95c1ffa7a494a45292574`; tested head `cb0b9da560672fec3e828cecf22afaf284b4bff9` | Manual Ubuntu/Docker/Compose/real-filesystem/backup/crash NOT RUN |
| **PARTIAL** | **BENCH-SCALE-01 (2/5; #132 and #133 merged)** | Python disposable enumeration baseline and CTest registration; latest merge `88ec4632e984b2164ce863bd2906e7d56b4465dd` | Not production C scanner throughput; manual OS/Compose/filesystem/crash NOT RUN |
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

Each **opened feature-specific PR attempt** (merged, closed, abandoned) counts once under the same stable feature ID, including test-only refinements and renamed subtasks. The `docs/feature-budget.json` ledger is validated in required PR CI; the PR must declare `Feature-ID: CLI-CONFIG-01` and then record its actual assigned PR number in the ledger before checks can pass. A changed PR gets additional commits without consuming another slot. At five PRs (or earlier if accepted), close DONE/PARTIAL/BLOCKED with verified scope and explicit gaps and move to a different READY feature. Don't hide a sixth PR under a new milestone name or a generic security exception.

Separate *code exists*, *merged*, *CI pass at exact SHA*, *manual Ubuntu/Compose*, *real filesystem/backup/crash testing*, *security approval* and *RELEASED*. Keep unexecuted checks **NOT RUN**, NEVER "PASS". The existing separate hourly autopilot must select current READY from here and stop repeated PERF-01 test-only PRs. P0 FS-01B-FIX remains prohibited until its prerequisites are approved, independent of feature cap.

## Standing evidence and ownership rules

- Reconcile current GitHub main SHA, open PRs, merge SHAs, workflow runs and review protections **before** any change; an open PR takes precedence over creating duplicate work.
- For every material feature PR, update the ledger + this compact board + affected docs **in the same PR** and report UPDATED vs N/A; don't create repeated post-merge status-only PRs. Archive retains historical chronology; never overwrite it.
- CI is a gate, not a release certificate. Confirm exact-head checks; conditional TSan/Rust/scale work not triggered is NOT RUN.
- The owner-authorized autopilot may merge only when current-head tests, security and independent review/branch protection are satisfied, with an expected-head safeguard; never deploy, move real customer data, publish or spend.
- C17 production and C11/C23 portability remain; Rust, FCJ3/FCJ4 and API/ABI concepts retain test-only/proposed boundaries.
