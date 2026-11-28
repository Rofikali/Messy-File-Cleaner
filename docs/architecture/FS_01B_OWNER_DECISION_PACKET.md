# FS-01B-FIX — Owner decision packet (design only)

**Status: PROPOSED / NOT APPROVED.** Review baseline main `75dec9537e3b8aa16c0a3214755ff9f7cd806b31`, 2026-11-28. This is a bounded design-only milestone under the existing FS-01B-FIX feature budget; no production behavior, privileges, journal format, or release authority changes. Implementation and REL-01 remain BLOCKED.

## Source-backed decision

At this baseline, `src/mover/mover.c` retains a source-parent UID/mode guard, opens the source with O_NOFOLLOW, copies from an open descriptor, publishes without replacement, syncs the destination directory and records PUBLISHED before calling retire_source. Retirement renames into a source-parent .fc-retire-* entry, checks inode/type with fstatat, then unlinks that name and syncs the parent. The known-risk regression is not a positive security pass. An inode pinned by an open descriptor does not make the later name-based unlink conditional on that inode. The current guard does not exclude a fully capable same-UID writer. A private 0700 directory alone does not exclude that writer either.

The existing proposed [ADR](ADR_FS_01B_SOURCE_RETIREMENT_BOUNDARY.md) remains unapproved. The [positive acceptance plan](../quality/FS_01B_FIX_SECURITY_ACCEPTANCE_PLAN.md) is still required. Existing recovery is pathname-based and does not certify object identity. A destination temporary-name cleanup also uses unlinkat in failure paths; audit its separate authority boundary before claiming a whole-mover preservation guarantee. No new exploit test or remediation performed in this PR.

## Recommended owner decision (not an approval)

| Decision | Recommended starting scope | Consequence |
| --- | --- | --- |
| Adversary | Include noncooperating same-UID namespace writers; do not claim protection from root or a compromised process | Do not authorize legacy name-based destructive retirement or a 0700-only quarantine as a complete fix |
| Unsafe authority | Refuse destructive moves before any mutation when authority is not established; retain preview | No false COMPLETED result; no silent change of move into copy |
| Optional copy-only operation | Separately named, explicit opt-in; design destination/temp authority, no-clobber, durability and truthful reporting before activation | Source preserved by this operation; duplicates and additional storage; not a whole-system attacker guarantee |
| Filesystem scope | Ubuntu 24.04 on explicitly validated local filesystems; no unsupported remote/overlay/idmapped/power-loss claims | Evidence matrix and manual operator signoff required |
| Recovery | No automatic deletion of uncertain entries; preserve published destination; explicit operator reconciliation | Prefer recoverable duplicates to uncertain deletion |

Alternative: if owner deliberately narrows the adversary model, the pinned quarantine design may be considered subject to exclusive authority and crash/orphan proof. Document the excluded threats conspicuously. No claim that ordinary Linux renameat2, openat2, statx, Landlock or additional identity checks supply an atomic inode-conditional unlink.

## Required next implementation decision

1. Owner explicitly accepts or changes the threat model, fail-closed behavior, supported filesystems and recovery rules. A routine go-ahead is not the specific production approval.
2. Review current mover API and CLI entry points, including direct library calls; prove guard cannot be bypassed through an alternative entry point. Decide whether initial safe release is preview-only, or a separately approved copy-only mode. No activation in this PR.
3. Specify states before publication, after durable PUBLISHED, on retirement refusal, after crash and on retry. Never mislabel a preserved source as a successful move. Recovery must not delete uncertain names; FCJ3/FCJ4 production migration is separately gated.
4. Define positive tests on disposable fixtures only: source and unrelated bytes/inodes survive; existing destination collision untouched; failed/uncertain states truthful; repeated recovery safe; supported filesystem matrix; no production testing hooks. Test destination temp cleanup separately.
5. Only after explicit scope authorization implement the smallest change, record exact head evidence and manual backup/restore/operator acceptance. No tag, distribution or customer-file operation until REL-01.

## Tradeoffs and review

L1–L7: prefer bounded, auditable refusal and honest API behavior over extra check/use calls. CISSP: same-UID threat and ownership are explicit; no root/privilege workaround. Linux/SRE: preserve existing no-replace publication and sync sequencing; test actual local kernel/filesystem and restore procedure. CCIE: no networking requirement. CA/CFO: copy-only adds potential disk consumption; measure bytes, retries and support effort; no authorized spend or assumed ROI. MBA/management: preservation is the primary customer value; preview-only may limit adoption but reduces data-loss exposure. No unsupported performance promises.

**Evidence status:** source inspection only; positive tests, local compilation, manual Ubuntu/Compose and filesystem/crash acceptance NOT RUN. PR CI, when available, is not approval or proof of this design. Rollback: revert this documentation; runtime unchanged. The original ADR and release gate remain authoritative until an explicit accepted replacement.
