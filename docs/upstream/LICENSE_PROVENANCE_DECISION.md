# LICENSE-01 — Licensing and provenance decision packet

**Status: PROPOSED / NOT CLEARED FOR REDISTRIBUTION.** Baseline main `a5c845c0cb934cafcde50021f17ac9cc6b204079`, inspected 2026-11-28. No license selected or granted, code import, dependency, tag, package publication, or release authorization. FS-01B-FIX and REL-01 remain blocked.

## Observed baseline and inventory limits

- Exact-ref fetches of root LICENSE, LICENSE.md, COPYING and NOTICE returned HTTP 404. This is a limited filename check, not a complete recursive audit or proof of ownership.
- [Upstream registry](upstream-registry.json) reports PENDING_OWNER_DECISION, external imports BLOCKED, and an empty component list. This is a recorded status, not proof that no historical third-party implementation exists.
- CMake declares Threads for the C runtime; Python3 interpreter and private library experiments occur in the test configuration. Dockerfile uses ubuntu:24.04 in build/runtime, and build-essential, cmake and ninja-build for compilation. Existing [supply-chain evidence](../security/SUPPLY_CHAIN_REFERENCE.md) reports pinned action declarations, mutable base/runner/APT and a local-only Rust test crate at its earlier snapshot. Exact transitive packages, licenses, linked binaries, Cargo and container contents require fresh artifact-level verification; this packet does not certify them.
- The [reuse policy](UPSTREAM_REUSE_POLICY.md) distinguishes independent implementation and API study from imported source. Existing Linux-inspired C research and native API references are not a blanket claim of original authorship or kernel code ownership. Review source history, contributors, generated code, copied snippets and notices before asserting rights.

## Owner choices (options, not selected terms)

| Option | Distribution implications | Tradeoff |
| --- | --- | --- |
| MIT | Permissive; preserve required notice | Simple adoption; no explicit patent grant comparable to Apache-2.0 |
| Apache-2.0 | Permissive, express patent terms and notice obligations | Useful for SDK adoption if provenance and compatibility clear; not generally compatible with GPL-2.0-only combined distribution |
| GPL-2.0-only | Copyleft obligations for covered combined distributions | Suitable only if ownership, component compatibility and open-source distribution strategy align |
| Proprietary / separately negotiated terms | Owner retains control of owned material; third-party obligations still apply | Requires tailored terms and rights verification; no automatic permission to incorporate incompatible code |

Commercial use is possible under open-source licenses; charging fees and exclusive rights are different questions. No dual-license strategy without sufficient rights. Prefer evaluating Apache-2.0 for independently owned code if broad commercial SDK adoption is the goal, but keep the choice pending until history and legal review support it. No license text installed by this PR.

Linux kernel distribution is generally GPL-2.0-only with an explicit syscall exception for relevant UAPI use. Normal native API use does not by itself impose the kernel license on an application, but the exception does not automatically authorize copying internal kernel code. Check exact per-file SPDX terms, exceptions and derivative-work/combination questions; static or dynamic linkage alone does not settle compatibility.

## Evidence required before release or import

For each shipped component record source URL, exact version/commit, paths and hash, copyright, SPDX expression and exceptions, required notices, changes, build/runtime/test scope, distribution/linkage relationship, obligations and review decision. Inventory the actual binary dynamic dependencies, any static objects, package files, base image and transitive packages. Keep test tools separate from runtime; container redistribution requires its own inventory. Record unresolved provenance as BLOCKED, never guess a license. Maintain SBOM and license notices for the exact candidate; validate no unapproved source import. Document owner approval of license and redistribution scope separately from REL-01 safety approval. No external publication during this phase.

## Review and evidence

Sources: [GitHub licensing guidance](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository), [Linux kernel licensing rules](https://docs.kernel.org/process/license-rules.html), [OSI license texts](https://opensource.org/licenses). Legal review and ownership confirmation remain pending; this is engineering/commercial planning, not a legal opinion. L1–L7: reproducible provenance and narrow scope; CISSP/SRE: supply-chain integrity and verified artifacts; CCIE: no networking change; CA/CFO: no authorized spend, review/maintenance costs unmeasured; MBA/management: licensing affects SDK adoption and commercial options, not automatic revenue. Local artifact inventory, exhaustive history review, legal signoff and manual release acceptance NOT RUN. Rollback by reverting this design documentation; runtime unchanged.
