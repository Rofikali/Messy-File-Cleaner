# Messy-File-Cleaner — Compact LIVE NOW / NEXT / LATER

> **Handoff (2026-11-22; verified GitHub):** PR #143 AUTOPILOT-HANDOFF-01 **MERGED** at `80de73b465ede49eec0882846c50efc0cbdee254`, reviewed exact head `c2c232aa7aceebf26c776a01f403fd3ac0d8ff77`; CMake, Docker, Makefile and Ubuntu 24.04 workflows succeeded on the PR head and merge commit. Its offline selector check does NOT establish live GitHub state. PR #142 previously merged scanner identity checking, which does NOT close source-retirement TOCTOU. **NEXT SELECTED FEATURE: PLAN-REPORT-01 (0/5)** — first verify existing dry-run behavior, then add a scoped opt-in *read-only* planning/reporting capability and preservation tests. Manual owner Ubuntu/Compose, hostile filesystems, backups and power-loss acceptance NOT RUN; FS-01B P0 and REL-01 remain BLOCKED.
>
> **One live selector:** this document, cross-referenced by the [advanced feature roadmap](docs/ADVANCED_FEATURE_ROADMAP.md), [acceptance matrix](docs/ADVANCED_FEATURE_ACCEPTANCE.md), [feature budget ledger](docs/feature-budget.json), [five-PR feature-delivery policy](docs/FEATURE_DELIVERY_AND_ACCEPTANCE_POLICY.md) and [history](next-history.md). Roadmap proposals are NOT another active queue; live GitHub supersedes dated snapshots.

## LIVE NOW / NEXT / LATER

| Lane | ID / budget / status | Concrete deliverable | What is NOT proven |
| --- | --- | --- | --- |
| **NOW — SAFE FEATURE SELECTION** | **PLAN-REPORT-01 (0/5; no PR yet)** | Inspect existing `--dry-run --once` output; bounded opt-in structured read-only plan/conflict reporting with no source/destination/journal mutation, positive preservation and hostile-input tests; [acceptance](docs/ADVANCED_FEATURE_ACCEPTANCE.md) | Not implemented; detailed behavior and JSON schema require review; FS-01B P0 and release remain blocked |
| **DONE** | **AUTOPILOT-HANDOFF-01 (1/5; #143 merged)** | Offline live-board/feature-ledger mismatch guard and negative tests; merged `80de73b465ede49eec0882846c50efc0cbdee254`, tested head `c2c232aa7aceebf26c776a01f403fd3ac0d8ff77`, four workflow families PASS | CI cannot verify live GitHub PR state; manual OS/Compose, crash/power-loss NOT RUN |
| **DONE** | **SCAN-ENTRY-IDENTITY-01 (1/5; #142 merged)** | Opened-directory identity check, merged `69fdffe49e2bf502e178fe3a92044d04f4f2653c`; tested PR head `dd7a07a09f6d819d2b57f22b2a15a727203fb2e1`; four post-merge workflow families PASS | Pathname TOCTOU and source-retirement P0 unresolved; manual OS/Compose/crash NOT RUN |
| **DONE** | **CLI-USAGE-SAFETY-01 (1/5; #141 merged)** | Fixed usage label at `85302ffa3a9749930c6f9513c3985a8b3d64d45f`; tested head `e5d4ff71237ffcd449e386c378845013c0420a38`; four workflow families PASS, TSan NOT RUN | Manual OS/Docker/Compose/real-filesystem/backup/crash NOT RUN |
| **DONE** | **CLI-VALUES-01 (1/5; #140 merged)** | Missing/empty CLI operands rejected at `89e3914603352d094ebca221e6b2bd4770457f9d`; tested head `1fd9cb537d0619ccafe6877a95752e77445fda6e`; five workflow families PASS | Manual OS/Docker/Compose/real-filesystem/backup/crash NOT RUN |
| **DONE** | **CLI-ERROR-01 (1/5; #139 merged)** | Bounded non-echoing diagnostics merged at `7bf033b18570adca94c42186c50fa0676b159284`; tested head `f01c074aaf7cb14b6e7749f175dea42019f2444e` | Manual Ubuntu/Docker/Compose/real-filesystem/backup/crash NOT RUN |
| **DONE** | **CLI-HELP-01 (1/5; #138 merged)** | Side-effect-free standalone help merged at `ced2ed12e44a1c0433d6d3d583b883f6f5e954e4`; tested head `36aae2003e36d363430723c5809b5160d7906435` | Manual Ubuntu/Docker/Compose/real-filesystem/backup/crash NOT RUN |
| **DONE** | **CLI-CONFIG-01 (1/5; #137 merged)** | Repeated `--config` rejected at `5d7a58f7f49b4db9d965e16a94254590b29cdb51`; tested head `6848a6f9f7a377c82a79bf178b923c7baaebf9ae` | Manual Ubuntu/Docker/Compose/real-filesystem/backup/crash NOT RUN |
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

## Future advanced stages — NOT active work

The owner's requested long-term [advanced roadmap](docs/ADVANCED_FEATURE_ROADMAP.md) and [acceptance/review matrix](docs/ADVANCED_FEATURE_ACCEPTANCE.md) record **candidate** features, not completed work or permission to start blocked operations. Planned sequence: S0 data-preservation and recovery security; S1 advanced preview/rules/conflict reporting; S2 duplicate and metadata analysis; S3 watcher resynchronization; S4 bounded-memory scanner and measured scale; S5 reviewed C static/shared SDK and Rust FFI; S6 optional TUI/desktop; S7 licensing/distribution/business. **PLAN-REPORT-01** is the single currently selected safe task; future candidates become READY only after prerequisite, risk, architecture and owner-authority review. See [related product/business assumptions](docs/PRODUCT_AND_BUSINESS_STRATEGY.md), [FS-01B design](docs/architecture/ADR_FS_01B_SOURCE_RETIREMENT_BOUNDARY.md), [API/ABI proposal](docs/architecture/C-API-ABI-FFI-ROADMAP.md), and [engineering operating model](docs/ENGINEERING_OPERATING_MODEL.md).

## 2026-11-21 completion and evidence snapshot

| Area | IMPLEMENTED / MERGED | Evidence observed | Remaining acceptance |
| --- | --- | --- | --- |
| C17 CLI, scanning/classification, queue/worker, legacy mover and journal | Existing code on main | C11/C17/C23 CMake/CTest, Makefile and Ubuntu 24.04 on main | Known source-retirement race; no approval for valuable files |
| FCJ3/FCJ4 parser/writer/testing | Isolated test-domain features, **not** production recovery | Source, tests and conditional CI lanes | No production FCJ4 link, trusted recovery or FFI |
| Disposable benchmarking and fixture refusal | PERF-01 work merged through #128 | CMake, Docker, Makefile, Ubuntu, performance workflows all success at `3d7e14ed1604` | No claimed nanosecond end-to-end latency, target-host million-file workload or power-loss test |
| Docker and developer tooling | Dev/prod Docker images, safe dry-run Compose default, guarded explicit execution, Makefile status/self-check/rust-check | Docker, Makefile and Ubuntu CI success at referenced main | Owner-run Ubuntu/Docker Desktop/Compose and external host filesystem tests NOT RUN in this audit |
| Release / real user safety | No approved release | Current repository remains prototype/development | FS-01B-FIX and REL-01 BLOCKED; no automated real file moves |

## Five-PR continuation rule (hard ceiling, not five PRs mandatory)

Each **opened feature-specific PR attempt** (merged, closed, abandoned) counts once under the same stable feature ID, including test-only refinements and renamed subtasks. The `docs/feature-budget.json` ledger is validated in required PR CI; a future feature PR must declare the current stable `Feature-ID: PLAN-REPORT-01` (or whichever feature is active at that time) and then record its actual assigned PR number in the ledger before checks can pass. A changed PR gets additional commits without consuming another slot. At five PRs (or earlier if accepted), close DONE/PARTIAL/BLOCKED with verified scope and explicit gaps and move to a different READY feature. Don't hide a sixth PR under a new milestone name or a generic security exception.

Separate *code exists*, *merged*, *CI pass at exact SHA*, *manual Ubuntu/Compose*, *real filesystem/backup/crash testing*, *security approval* and *RELEASED*. Keep unexecuted checks **NOT RUN**, NEVER "PASS". The existing separate hourly autopilot must select current READY from here and stop repeated PERF-01 test-only PRs. P0 FS-01B-FIX remains prohibited until its prerequisites are approved, independent of feature cap.

## Standing evidence and ownership rules

- Reconcile current GitHub main SHA, open PRs, merge SHAs, workflow runs and review protections **before** any change; an open PR takes precedence over creating duplicate work.
- For every material feature PR, update the ledger + this compact board + affected docs **in the same PR** and report UPDATED vs N/A; don't create repeated post-merge status-only PRs. Archive retains historical chronology; never overwrite it.
- CI is a gate, not a release certificate. Confirm exact-head checks; conditional TSan/Rust/scale work not triggered is NOT RUN.
- The owner-authorized autopilot may merge only when current-head tests, security and independent review/branch protection are satisfied, with an expected-head safeguard; never deploy, move real customer data, publish or spend.
- C17 production and C11/C23 portability remain; Rust, FCJ3/FCJ4 and API/ABI concepts retain test-only/proposed boundaries.
