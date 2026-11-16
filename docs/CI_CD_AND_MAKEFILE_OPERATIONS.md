# CI/CD, Makefile and Evidence Operations

**Status: factual workflow inventory + proposed process, 2026-11-16; reviewed `main` at `559ba1e`.** This document does not change GitHub Actions, branch rules, code, Docker runtime or release. Reinspect exact current workflow content and required checks at PR SHA before claiming coverage.

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

`rust-toolchain.toml` and the test-only `tests/rust/fcj4-test-support/Cargo.toml` are **not currently wrapped by root Make targets**. Use `cargo fmt --all -- --check`, `cargo clippy --locked --all-targets -- -D warnings`, `cargo test --locked` from that package. A future `make rust-check`, `make status` and `make self-check` is an appropriate **separate** bounded implementation PR with refusal tests; do not claim these targets already exist.

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
