# CI/CD, Makefile and Evidence Operations

**Linux-native next research (2026-11-28):** #302 merged at `41ed88fdef6dd6ed548564f30b7ddb667f776969`; 14/14 exact-head checks PASS, GCC C17 Release and hosted Ubuntu C17 85/85, all three Landlock references PASS (not skipped); integrated-main 8/14 checks PASS, 6 pending at inspection; independent review unverified. LANDLOCK-PREVIEW-EVAL-01 fourth candidate tests inherited restriction in a forked descendant and an unrestricted parent control using only a disposable fixture. No production sandbox, universal descriptor audit, same-UID isolation, kernel source import or FS-01B/release closure. Candidate CI pending. [Hypotheses and evidence](upstream/LINUX_NEXT_HYPOTHESES.md).

**Owner-directed release and distribution phase (2026-11-27):** [Release, packaging and distribution roadmap](RELEASE_PACKAGING_DISTRIBUTION_ROADMAP.md). `go ahead` / `continue` / `next` / `next milestone` resumes the next eligible bounded job in the live board after GitHub reconciliation; same rules for hourly agents and engineers. CI-RECONCILIATION-01 is selected for the release-baseline incident; FS-01B-FIX remains the critical blocked safety prerequisite. LICENSE-01, PACKAGING-01, RELEASE-PIPELINE-01, GHCR-01, REL-01, PRODUCTION-SDK-01 and PRODUCT-ADOPTION-01 are gated future work, not implemented or release authorization. Preserve five-PR ceilings, no-spend, no external import, no automatic tag/publish, and the existing Rust FFI gate. Apply L1–L7 engineering, CISSP/security, CCIE/networking, Linux/POSIX, SRE/DevOps, CA/CFO, MBA/product and management review lenses; synchronize core and affected docs with evidence, not claims. The initial known main failure is Ubuntu C23 APT install (403), not a compiler failure; do not disable signature checks. Package builds remain unpublished until exact-version release approval.

**Final upstream adoption decision:** [Evidence-first gate](upstream/UPSTREAM_ADOPTION_DECISION.md). #296 adds ASan/UBSan in Debug for the existing private TCO test target; no new workflow or toolchain. Exact-head CI pending.

**Cash versus noncash TCO:** [Evidence and decision contract](upstream/UPSTREAM_TCO_EVIDENCE.md). #295 extends the existing private CTest with ten synthetic cash/noncash separation checks; no workflow or toolchain changes. Exact-head CI pending.

**Bounded upstream TCO sensitivity:** [Evidence and decision contract](upstream/UPSTREAM_TCO_EVIDENCE.md). #294 extends the existing private CTest with 12 synthetic bounded-horizon checks; no workflow or toolchain changes. Exact-head CI pending.

**Checked upstream TCO reference:** [Evidence and decision contract](upstream/UPSTREAM_TCO_EVIDENCE.md). #293 registers a private CTest integer TCO reference with 17 synthetic checks; exact-head CI pending. No billing ingestion or new dependencies.

**Upstream maintenance cost evidence:** [TCO baseline](upstream/UPSTREAM_TCO_EVIDENCE.md). #291 restored Ubuntu validation; 13/13 returned latest jobs passed after retry. #292 records selected log spans and CTest times, not billable minutes; no CI configuration change.

**Final upstream maintenance evidence:** [Master plan](upstream/UPSTREAM_MASTER_PLAN.md). #291 repairs the malformed Ubuntu 24.04 workflow tail introduced in #278, restores static sanitizer execution, retains existing checks and adds verbose capability/shared sanitizer evidence. Structural check only; exact-head CI pending.

**Upstream capability reporter controls:** [Master plan](upstream/UPSTREAM_MASTER_PLAN.md). #290 CTest exercises mixed capability outcomes, five failure controls, required-probe enforcement and actual timeout; CI pending.

**Upstream capability correlation:** [Master plan](upstream/UPSTREAM_MASTER_PLAN.md). #289 adds a Linux CTest correlation report for six environment fields and five existing disposable probes, bounded timeouts, explicit skipped/not-built states and failure propagation. CI pending.

**Upstream environment drift comparison:** [Master plan](upstream/UPSTREAM_MASTER_PLAN.md). #288 CTest compares raw six-field environment reports, including actual self-comparison and synthetic drift, reorder and malformed controls; no network or update. CI pending.

**Upstream maintenance environment evidence:** [Master plan](upstream/UPSTREAM_MASTER_PLAN.md). Read-only six-field environment CTest report; verbose CTest/LastTest.log exposes observed kernel and toolchain identities, not security or capability conclusions. CI pending.

**Private Rust FFI feasibility proposal:** [Design and approval gate](architecture/RUST_CLASSIFIER_FFI_FEASIBILITY.md). Design-only proposal; no CI build configuration, toolchain or code change. Future isolated offline pinned Rust tests and C evidence require explicit owner scope approval and staged ADR review.

**Final private shared-library acceptance:** [Linux-inspired architecture](architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). Final Release CTest checks no private DSO dependency in production or runtime-only consumer, three absent targets, both CLI-only stages and three negative controls; no new dependencies. CI pending.

**Private shared sanitizer and ownership:** [Linux-inspired architecture](architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). Debug instrumented DSO and both consumers with fatal ASan/UBSan/leak checks; Clang Debug alone permits host-resolved sanitizer hooks; Release linker/ELF gates unchanged. CI pending.

**Private shared runtime and hardening:** [Linux-inspired architecture](architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). Existing CTest matrix adds private runtime loader cycles; Release ELF gate inspects RELRO, eager binding and non-executable stack. CI pending.

**Private shared ELF boundary:** [Linux-inspired architecture](architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). Release CTest inspects dynamic exports, private SONAME, relative consumer RUNPATH, NEEDED and text relocations, with two disposable negative controls; existing binutils only. CI pending.

**Private shared library reference:** [Linux-inspired architecture](architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). Private PIC DSO and separate consumer use existing CTest matrix, hidden default visibility and linker resolution gate; no install/export or new dependency. CI pending.

**Final private static archive acceptance:** [Linux-inspired architecture](architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). Release CTest final artifact gate checks five symbols, fresh BUILD_TESTING=OFF build and missing private targets, CLI-only staged install and two negative controls; no added dependencies; CI pending.

**Static archive matrix and ownership:** [Linux-inspired architecture](architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). Release CTest adds the existing dependency oracle to the normal matrix; Debug instrumented consumer exercises caller-owned heap storage. CI pending.

**Static archive dependency and sanitizer boundary:** [Linux-inspired architecture](architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). Existing Clang C17 Release step checks per-member undefined dependencies and two disposable negative controls; Debug archive and consumer ASan/UBSan enabled; hosted Clang leak-check rerun. CI pending.

**Static archive symbol boundary:** [Linux-inspired architecture](architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). Existing isolated Clang C17 Release step adds nm definition allowlist and two disposable archive negative controls; CI pending.

**Owner-approved Linux-inspired library architecture (2026-11-27):** [Engineering requirements](architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). #276 adds independent same-environment Clang C17 Release archive comparison (ar member order, cmp and SHA-256), preserves strict warnings after EOF-only corrections; pending exact-head CI.

**Private static archive reference:** [API roadmap](architecture/C-API-ABI-FFI-ROADMAP.md). Existing CMake/CTest matrix builds and exercises a private STATIC target only when BUILD_TESTING is enabled; no install/export or production CLI linkage. CI pending.

**Public API final isolation candidate:** [API roadmap](architecture/C-API-ABI-FFI-ROADMAP.md). Existing isolated Clang production-only C11/C17/C23 builds check symbol and staged CLI-only install isolation with positive mock and two negative controls per mode; no new runner or package. CI pending.

**Public API synthetic ownership:** [API roadmap](architecture/C-API-ABI-FFI-ROADMAP.md). Existing six compiler modes retain synthetic caller checks; existing hosted Clang 18 job adds ASan/UBSan and leak detection, no new runner or dependency. CI pending.

**Public API mock caller:** [API roadmap](architecture/C-API-ABI-FFI-ROADMAP.md). Existing isolated Ubuntu development image runs the separate synthetic mock caller under GCC/Clang C11/C17/C2x; ten scenarios per build, no real engine, new runner or dependency. CI pending.

**Public API consumer boundary:** [API roadmap](architecture/C-API-ABI-FFI-ROADMAP.md). Existing six strict syntax probes compile the revised consumer only; no job, runner, dependency, linking or runtime test added.

**Public API design fixture:** existing Ubuntu image job compiles a test-only header consumer across GCC/Clang C11/C17/C2x with -fsyntax-only and -Werror; no link, execution or install.

**Explicit capability outcomes:** existing Clang Release and Debug steps rerun five exact-name CTest fixtures with verbose output; skipped means NOT_VERIFIED and any missing/failing/ambiguous outcome fails report. No new jobs.

**Internal header syntax gate:** existing Ubuntu image job runs standalone duplicate-inclusion checks with GCC/Clang, three language modes and test macros; no extra runner/packages.

**Clang Debug correction:** move only Debug step to existing hosted Ubuntu runner with preinstalled clang-18 and explicit sanitizer link/run probe; original container Release isolation unchanged; host Debug not container-isolated.

**Clang Debug suite:** existing Ubuntu development image, same isolation, eight-minute step, target-specific sanitizer instrumentation, halt-on-error and full registered CTest suite. No new runner or packages.

**Clang C17 Release suite:** reuse the existing development container with the same isolation, eight-minute step, actual registered CTest suite and no exclusions. No new runner/packages.

**Clang production compatibility:** existing Ubuntu container-build job reuses development image for three network-isolated read-only-source Clang builds and CLI help tests; five-minute step, no new runner/packages.

**Direct pin enforcement and regression:** existing feature-budget job tests actual inventory gate with 11 isolated temporary Git fixtures, then checks real tracked references. No extra runner or remote workflow execution.

**Action reference pinning:** existing nine workflows use four official full-SHA action references in 18 declarations; same inventory and review update policy. No new jobs or manual scale dispatch.

**Build reference drift gate:** existing feature-budget job checks direct literal workflow/Dockerfile declarations against reviewed inventory; no new runner, packages or workflow.

**APT retry regression:** existing feature-budget job runs isolated shell oracle with seven deterministic cases; no real APT/network in the test.

**APT mirror retry:** [build integrity](security/SUPPLY_CHAIN_REFERENCE.md) records exact failed Docker job and successful retry, adds bounded clean strict index refresh in production-image build; package authentication remains enabled.

**Acquisition fault evidence:** isolated config stream test now wraps open as well as fdopen/malloc/free; direct Debug sanitizers and verbose Release retained.

**Allocation fault evidence:** isolated config stream test target now wraps malloc/free in addition to fdopen; direct Debug sanitizers and C17 Release verbose test retained.

**Fault evidence:** Linux GNU/Clang isolated config stream test uses linker wrap only in test target, direct Debug sanitizer and C17 Release verbose CTest.

**Linux scanner exec evidence:** C17 Release matrix emits verbose `linux_scanner_cloexec` output; existing timeout and direct Debug sanitizer checks retained.

**CPU reference evidence:** C17 Release matrix leg emits verbose `cpu_opt_reference` output and `objdump` of `scalar_count`; GNU Release reports auto-vectorization diagnostics. No extra workflow dispatch.

**Cache-layout evidence:** existing C17 Release verbose reporting also selects cache_layout_reference exactly; reports representation sizes and verified content equivalence. No new runner, dependency or scale dispatch.

**Batch I/O evidence:** existing C17 Release verbose reporting also selects batch_io_reference exactly, exposing actual syscall counts and verification; no new runner, workflow or scale dispatch.

**io_uring capability evidence:** existing C17 Release verbose reporting also selects linux_uring_capability exactly; build/runtime capability skip is NOT VERIFIED, never an executed operation pass. No new runner, dependency install or security-policy change.

**Linux I/O evidence:** existing C17 Release reporting step also selects linux_io_reference exactly and exposes verbose rows; missing test fails the evidence step, skipped capability remains NOT VERIFIED. No new workflow or scale dispatch.

**Scanner reference evidence:** CMake matrix C17 Release only runs the existing bounded scanner reference smoke once more with verbose output and kernel/compiler/libc context. Exact test selector and --no-tests=error ensure missing test fails. No additional workflow or scale dispatch. See [baseline](performance/SCANNER_REFERENCE_BASELINE.md).

**CI scope:** existing required Five-PR feature delivery contract job also runs the upstream import-lock validator and unit tests. No extra runner/job, network dependency, production Python or branch-rule context. Structural guard cannot detect arbitrary copied snippets or all linkage; exact-head CI and human/agent source review remain necessary.

**Owner-approved upstream-first policy (2026-11-25):** Follow [reuse policy](upstream/UPSTREAM_REUSE_POLICY.md) and [master plan](upstream/UPSTREAM_MASTER_PLAN.md). Prefer supported Linux/POSIX APIs and mature compatible components; retain working code unless comparative evidence justifies change. External source imports and new third-party dependencies are BLOCKED pending explicit owner license decision and component acceptance. C17, C11/C23 CI, FS-01B/REL-01, no-spend and five-PR constraints remain binding.

**Status: factual workflow inventory + proposed process, 2026-11-16; reviewed `main` at `559ba1e`.** This document does not change GitHub Actions, branch rules, code, Docker runtime or release. Reinspect exact current workflow content and required checks at PR SHA before claiming coverage.

## 2026-11-21 documentation and feature-budget gate

A new stdlib-only Python policy check runs in the existing CMake workflow on PRs and pushes: `python3 -m unittest discover -s scripts/tests -p 'test_feature_budget.py'` plus `python3 scripts/feature_budget_check.py --event "$GITHUB_EVENT_PATH"`. This enforces **at most five opened PR numbers per feature**, one active feature, and a PR's body `Feature-ID` and actual ledger registration; it cannot replace live GitHub reviews or validate user files. No Python is linked into the production C17 binary. CMake/CTest, Linux, Docker, Makefile, Rust/TSan checks remain independently scoped. New PR policy checks are pending until executed on the changed head; previous CI success cannot be copied to it.

## 1. Build authority and safe command menu

**CMake is the C build source of truth. The root Makefile is a thin local command interface.** Do not copy IMGENGINE's Rust-Axum/Nuxt/Playwright targets: the file cleaner has neither production web nor browser code.

| Command | Actual effect today | Safety/review note |
| --- | --- | --- |
| `make help` | Print supported commands | Read first |
| `make` / `make debug` | Build Debug with ASan/UBSan | Current default is a build, unlike IMGENGINE's help default; any future change requires separate review |
| `make test` | Debug build + CTest | Bounded `JOBS=2` default |
| `make validate` | Separate C11, C17, C23 Debug builds + CTest | Does not replace Release matrix or PR CI |
| `make release` | Portable optimized C17 Release build | **Not** a certified distribution release |
| `make run` | Debug `--dry-run` | Must not mutate customer files |
| `make run-execute CONFIRM_MOVE=YES` | Explicit real file operations | Treat as destructive; review input root/config first |
| `make benchmark` | Dry-run benchmark harness | Records workload/environment; not production latency |
| `make benchmark-moves` | Real moves only in isolated generated fixtures | Inspect fixture path behavior |
| `make profile`, `make perf`, `make gprof`, `make asm` | Diagnostic build/measurements | No assembly deployment promise |
| `make clean` | Deletes specified build/profiling files | Review ownership of deleted paths; do not use on customer data |

The root Makefile now provides `make rust-check`, `make status`, and `make self-check` (verified target names on 2026-11-21); inspect each recipe and CI evidence before inferring its guarantees. `rust-check` is **test-only** Rust and is not production C/Rust linking.

## 2. Current GitHub Actions inventory

| Workflow | Trigger actually configured | What it verifies | Not proof of |
| --- | --- | --- | --- |
| `cmake-single-platform.yml` | Push main, PR to main | C11/17/23 × Debug/Release CMake/CTest | Every unsupported filesystem/power cut |
| `makefile-validation.yml` | Push main, PR | `make help`, `make test`, `make validate`, safe execute refusal and invalid JOBS check | Release readiness or arbitrary real paths |
| `ubuntu-24.04-validation.yml` | Push main; selected PR paths; manual | C11/17/23 Release tests, dev/prod Ubuntu images, non-root prod check | Docker Desktop, every libc/kernel/filesystem |
| `docker-build.yml` | Push main, PR to main | Container build; disposable dry-run readonly input and real move with writable input | Live host volume/permission correctness |
| `rust-test-support.yml` | Push main on Rust/tooling changes; PR on same paths; manual | Pinned Rust fmt, Clippy, unit tests | C/Rust FFI, Rust prod link, fuzzing |
| `fcj4-tsan.yml` | Selected relevant PR paths; manual | Focused Linux test-only append adapter concurrency under TSan | All schedules, production recovery |
| `performance-benchmark.yml` | Manual; main push on selected performance/build paths | Disposable scan/move results and artifacts | Absolute performance SLA |
| `large-scale-scan-benchmark.yml` | Manual | 5k/100k/1M file-size choices, resource/metadata capture | Automatically exercised PR test; 1M safe on every runner |

**There is no observed autonomous deploy pipeline.** No public release, API service, paid cloud or managed VPS is established by this workflow set.

## 3. Verification lanes (recommended operating policy)

- **L0 developer loop**: inspect diff, run focused affected test, a local status/Makefile sanity check when relevant. Avoid repeatedly streaming full passing sanitizer logs into agent context.
- **L1 PR/CI integration**: run all *currently required* GitHub checks on exact PR head. Matrix and Docker checks are broad; path-scoped Rust and TSan checks are conditional and must not be described as executed when not triggered.
- **L2 high-risk verification**: on journal/mover/path/concurrency changes, add adversarial and crash/fault-injection, sanitizer, race, cross-filesystem and non-root tests as applicable. Do not bypass these gates for speed.
- **L3 release gate**: exact candidate, reproducible artifact, license/distribution/backup policy, supported environment, performance/resource budgets, recovery and rollback sign-off. A green PR is not a release.

For GitHub CI, report exact head SHA, workflow, status/conclusion, link, and NOT RUN/SKIPPED explicitly. Keep successful logs in CI/artifacts; show bounded failed-step excerpts. Model tokens and GitHub Actions billed minutes are different budgets; minimize both without weakening required checks.

## 4. Safe Makefile/CI improvements — *future work, not done in this docs PR*

1. Add `make status` and `make self-check` to validate menu, JOBS range, dry-run guard and refusal of unconfirmed execute operations; decide whether the default `make` should remain backward-compatible Debug or switch to help in a dedicated PR.
2. Add `make rust-check` (and optional `rust-fmt`) that runs isolated test-support Cargo checks only; no production dependencies.
3. Ensure readonly dry-run input is explicit in Compose and tested without breaking the separately authorized writable-input real-move fixture.
4. Review path filters/branch protection **after** checking their effect; do not optimize CI by disabling required checks.
5. Pin external GitHub Actions to reviewed immutable commits and base container digest when supply-chain review warrants it; document update/rollback. Not implemented.
6. Evaluate `make verify` only if it adds clarity over existing `make validate` and CI; prevent expensive accidental default operations.
7. Keep benchmark-only 1M tests manual, bounded by runner disk/inodes/memory and explicit fixture cleanup.

## 5. Evidence checklist per PR

`BASE_SHA`, `PR_HEAD_SHA`, files changed, scoped tests, exact commands and exit statuses, selected CI runs, missing checks, production-link/packaging impact, security/failure cases, rollback, owner approval. If no actual execution evidence exists, say **NOT RUN**. Tests from prior PRs are historical baselines, not new evidence.

Companion policies: [Makefile safety](quality/MAKEFILE-SAFETY.md), [engineering model](ENGINEERING_OPERATING_MODEL.md), [continuation](AGENT_CONTINUATION_AND_HANDOFF.md), [next](../next.md), [threat model](security/THREAT_MODEL.md).

PR #224 same-PR repair: keep the anchored CTest selector and all flags on one YAML-indented command line; initial CMake workflow did not start jobs. Exact corrected-head execution required; no additional workflow dispatch or PR attempt.


PR #247 same-PR correction: both cache_layout_reference and cpu_opt_reference selectors must remain on single lines with complete anchored regex and shell quotes. Earlier exact head returned seven passing jobs but no CMake workflow; corrected-head matrix mandatory. No check removed or bypassed.
