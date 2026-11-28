# Release, Packaging, Distribution and Adoption Roadmap

**Sparse extent reference (2026-11-28):** #315 merged at `fb82c8ab64d53e0345c517b8cade6d2104cd7c2f`; four returned PR workflow families passed, independent review not returned. SPARSE-EXTENT-EVAL-01 first candidate: disposable 4 MiB sparse and buffered fixtures, two marker bytes, full logical byte oracle, allocated-block observation, bounded SEEK_DATA/SEEK_HOLE traversal and explicit capability skip. No performance, physical savings or production copier claim. Local NOT RUN; CI pending. FS-01B-FIX/REL-01 remain blocked.

**Next selected research:** SPARSE-EXTENT-EVAL-01 (0/5), distinct test-only sparse extent evaluation using SEEK_DATA/SEEK_HOLE on disposable Linux files. Compare logical bytes, allocated blocks, extent boundaries, unsupported capability and error cases against buffered baseline; reflink is a conditional later experiment only if scoped evidence justifies it. No production copy/mover change. Governance PR #315 is not a feature attempt. Exact-head CI pending; FS-01B-FIX and REL-01 remain blocked.

**Owner direction (2026-11-28):** Continually identify Linux kernel and mature userspace techniques even when absent from the roadmap. Record real unmet requirement, falsifiable hypothesis, baseline, negative controls, capability/portability limits, licensing/provenance, maintenance/TCO, rejection and rollback criteria before selecting one bounded feature. Prefer native interfaces and independent designs; no blanket source-copy approval, no production safety gate bypass, no spend or release. See the Linux hypotheses and upstream reuse policy.

**Live reconciliation (2026-11-28):** PR #314 merged at `22e11eabf60a5abfbd8073acb25ba685b83fd4ac`; exact head `242a2d5dc3a712e7317fddea7e42df88395c4451`, five returned PR workflow families successful (CMake, Ubuntu, Docker, Makefile, FCJ4 TSan). Individual jobs, integrated-main and independent review not verified. WATCH-OVERFLOW-EVAL-01 PARTIAL 5/5; retain fail-closed watcher, no sixth PR. FS-01B-FIX and REL-01 remain blocked.

**Linux-native next research (2026-11-28):** #313 merged at `c3d0d9513a3dff49ecb03d26e6c14e44f3d81d8e`; exact-head 14/14 checks PASS, GCC C17 Release and Ubuntu C17 96 passed/1 skipped (97 registered), all four Inotify references PASS (not skipped); integrated-main 12/14 checks PASS, 2 pending at inspection; independent review unverified. WATCH-OVERFLOW-EVAL-01 (5/5; PR #314 open) adds deterministic close/register gap creation after actual overflow and requires rejection of old snapshot despite an empty new queue. Adoption decision: retain production fail-closed watcher; defer automatic recovery. No sixth PR, production recovery, dispatch, source import, spend, release or FS-01B closure. CI pending. [Hypotheses and evidence](upstream/LINUX_NEXT_HYPOTHESES.md).

**Owner direction, 27 November 2026.** Future work requested after the A–Z audit. Planning authorization only: no license selection, publication, tag, package push, new FFI, external source import, paid service, or destructive file operation is authorized. [Live selector](../next.md) and [feature ledger](feature-budget.json) remain authoritative; this document is not a second execution queue.

## Verified starting evidence

Main at audit: `caf3056bc09854a8846a7409c91ba1ef1de893f3`; [PR #296](https://github.com/Rofikali/Messy-File-Cleaner/pull/296) merged. The five UPSTREAM-COST-01 attempts are exhausted; PARTIAL, no sixth PR. PR-head CMake, Makefile, Docker, Ubuntu and FCJ4 TSan passed; native scanner workflow failed. Integrated-main Docker, performance, CMake and Makefile passed; [Ubuntu run 45941044992](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/45941044992) C23 job failed before compilation due to Microsoft APT 403/exit 100. Do not weaken apt signature checking. Native scanner failure and million-file acceptance are separate unverified evidence; no automatic large benchmark dispatch.

CMake declares 1.0.0 but GitHub release and tag inventories were empty at audit. No recognized LICENSE file; Docker CI uses push: false. Package inventory not accessible through inspected connector; do not claim it is empty. Static/shared classifier libraries are private test-only references, not approved SDKs. FCJ4/Rust test support is not production FFI. FS-01B-FIX and REL-01 remain BLOCKED. See [security guard](security/FS_01B_SHARED_SOURCE_GUARD.md), [API roadmap](architecture/C-API-ABI-FFI-ROADMAP.md) and [business strategy](PRODUCT_AND_BUSINESS_STRATEGY.md).

## Ordered work and acceptance

Security is the highest product priority. Because the destructive correction requires separate approved trust/crash design, first select the independent CI incident below; prepare safety design evidence without bypassing gates. All feature IDs are stable, at most five opened PRs across authors, not five mandatory PRs. Existing FS-01B-FIX/REL-01, static/shared/API/FFI and benchmark budgets are preserved; do not rename exhausted work.

| Order / ID | Scope and deliverable | Acceptance / dependency |
| --- | --- | --- |
| 1 — CI-RECONCILIATION-01 | Diagnose exact Ubuntu APT failure and native scanner run; reconcile CI and live handoff | Signed package sources, bounded retries only where justified; actual relevant GCC/Clang C11/C17/C23 and Docker/Makefile checks; classify skip/failure without false pass; no unrelated feature polishing |
| 2 — FS-01B-FIX | Existing source-retirement trust/crash boundary; propose and review safe fix | Owner-approved design, no unrelated file removed, no clobber, same-UID race controls, journal and recovery tests; disposable fixtures only; supported filesystems and manual recovery evidence; no sixth prior feature PR |
| 3 — LICENSE-01 | Inventory original/upstream code, dependencies, copyright, SPDX and license options | Explicit owner decision and appropriate legal review before redistribution/import; no assumed MIT/Apache/GPL or dual-license grant |
| 4 — PACKAGING-01 | CMake/CPack local tar.gz and .deb candidate; x86_64 first, arm64 only after actual validation | Build from exact commit; clean install, upgrade, uninstall, dependencies, package contents and permissions; test on clean supported Ubuntu; no private test artifacts; no external distribution |
| 5 — RELEASE-PIPELINE-01 | Version/changelog, release candidate build workflow, checksums, SBOM, artifact attestations/signing and rollback | Least privilege, pinned dependencies, immutable source and artifacts, verify tamper rejection and reproducibility limits; dry-run only; no auto tag/release from hourly merges |
| 6 — GHCR-01 | OCI image build/test; future GHCR package ghcr.io/rofikali/messy-file-cleaner | Non-root runtime, read-only preview where appropriate, no sensitive mounts, digest/architecture scan and smoke tests; no push until REL-01 explicit GO and visibility/billing verification |
| 7 — REL-01 | Existing manual release approval gate; coordinated GitHub Release and GHCR publish | FS-01B safety, license, CI, backup/recovery/rollback, target host/Compose/filesystem acceptance, version and artifacts reviewed, explicit owner GO for exact scope; verify actual distribution only after approval |
| 8 — PRODUCTION-SDK-01 | Real consumer-backed C API/ABI, static .a and shared .so | Separate approved scope; preserve completed/exhausted private reference features; public headers, ownership/errors, compatibility and SONAME/symbol gates; Rust FFI remains blocked under its existing ADR/budget until separate approval |
| 9 — PRODUCT-ADOPTION-01 | User onboarding, support and business validation | First safe dry-run success, retention/feedback, defects, support time, cash/noncash TCO and customer demand; no private filename telemetry, paid plans or spend without approval |

## Proposed distribution contract (not existing artifacts)

A pre-1.0 developer preview such as v0.1.0-alpha.1 is a proposal, not an assigned release. Resolve the current CMake 1.0.0 mismatch in the packaging/version milestone; never tag automatically. A preview with unresolved destructive safety must be explicitly approved and technically incapable of destructive execution, not merely documented as dry-run. Existing REL-01 is not waived. Candidate assets: Linux executable archive, Debian .deb, SHA256SUMS, SPDX SBOM, provenance/attestation, release notes, source archives, optional validated benchmark evidence. ARM64 only after supported build and execution tests. Do not publish private .a/.so candidates.

GitHub Releases distributes downloadable assets; GHCR distributes OCI containers. GitHub Packages is not a native APT or C SDK registry. A downloaded .deb may later be installed with apt install ./package.deb; a direct apt install file-cleaner needs a configured signed APT repository or distribution acceptance. Own APT hosting and official distro submission are optional later decisions, not current scope. No VPS, DB, domain or cloud service required for the initial local CLI. Verify current official GitHub pricing/quotas and actual billing before publishing; public/free does not mean zero economic cost.

## Manual and hourly continuation keyword

`go ahead`, `continue`, `next`, and `next milestone` are owner continuation commands for authorized ChatGPT, Codex, other coding agents or engineers. They mean perform the next dependency-unblocked bounded job, not merely describe it. The hourly autopilot follows the same selector. Reconcile live main, open/recent PRs across authors, exact head/base CI, branch and feature ledger; resume existing work first. One active feature, five opened PRs maximum; same-PR fixes are not new slots. At exhaustion close scoped DONE/PARTIAL/BLOCKED honestly, then select independent READY work. If blocked, document concrete evidence and move only to an independent safe candidate; never bypass approval, review or protection. Ordinary CI-gated merge authority follows existing governance; no automatic production release, tag, paid spend, license choice, FFI, upstream import or real file deletion. Docs do not wake an external agent or alter its schedule.

## Multidisciplinary execution and review

| Lens | Required evidence |
| --- | --- |
| L1 implementation | Clean, bounded C17 code, compatible C11/C23 configurations, error and resource ownership |
| L2 engineer | Unit/integration/system tests and negative cases |
| L3 senior | DRY/SOLID where useful, maintainable boundaries, regression and rollback |
| L4 lead | HLD/LLD/ADR, integration and acceptance dependencies |
| L5 staff | Cross-module ownership, ABI and release contracts, measurable performance |
| L6 principal | Threat/crash model, portability, supply chain and lifecycle tradeoffs |
| L7 distinguished/architect | Strategy, long-term compatibility and architecture economics; no unsupported kernel-grade claims |
| Security/CISSP | Threat model, least privilege, data preservation, artifact integrity and incident response |
| Linux/POSIX and SRE/DevOps | GCC/Clang, filesystem semantics, Docker/CI, reproducible deployment, recovery, monitoring and rollback |
| Networking/CCIE | Registry/TLS and dependency fetch boundaries; no unnecessary network infrastructure |
| CA/CFO | Actual cash vs noncash TCO, cost and liability evidence, no invented ROI or spend |
| MBA/product/management | Customer problem, market validation, support capacity, priority, risks and go/no-go |

These are review lenses, not claims of professional certification or independent review. Update AGENTS.md, README.md, skills.md, SKILLS.md, next.md, ledger and relevant scoped docs within each substantive PR; preserve historical records; mark unchanged families N/A. Record exact source/PR/CI IDs, failures, NOT RUN, cost impact, rollback and next eligible work. Read [feature policy](FEATURE_DELIVERY_AND_ACCEPTANCE_POLICY.md) and [continuation](AGENT_CONTINUATION_AND_HANDOFF.md).
