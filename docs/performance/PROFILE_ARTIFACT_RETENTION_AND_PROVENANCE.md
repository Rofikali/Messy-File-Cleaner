# HYGIENE-01 — Profiling Artifact Provenance and Retention

**Status:** Audit and cleanup proposal only; reviewed remote `main` at `0184cc1b3fb8b70f599578a025bd134fec0c9618` on 2026-11-16. Do not interpret this document as a new benchmark or approval to delete historical evidence.

## Source findings

| Tracked artifact | Git tree size | Inspection | Risk/decision |
| --- | ---: | --- | --- |
| `gprof.txt` | 17,460 bytes | Text profile was read from GitHub. It contains an old build log with `/workspaces/Messy-File-Cleaner/` absolute development paths and a roughly 100,000-file execution sample. | **Historical only**: no exact current HEAD/runner/input provenance, not a reproducible present-day benchmark; absolute path and function names may disclose local environment. |
| `perf.data` | 526,942 bytes | GitHub lists this tracked binary blob, but contents were **not inspected** by this audit (connector cannot safely decode binary profile data). | Treat as potentially containing command lines, kernel/function symbols, filesystem paths and execution/environment metadata until privately inspected by owner. Do not assert safety, corruption or reproducibility. |

- The existing root `.gitignore` **already** ignores `/gprof.txt` and `/perf.data`, yet both remain tracked. Ignore rules do not remove files from Git's index or history.
- The current `Makefile` has `make perf-record` using `sudo perf record`, and `make gprof` using `gprof ... gmon.out | less`; `make clean` removes some generated artifacts. None of these commands automatically preserves verified benchmark provenance.
- Historical `gprof.txt` refers to an older CMake source list and old output paths; do not use its percentages to rank bottlenecks in the current code.
- Structured performance evidence already lives in [CI performance baseline](CI-BASELINE-2026-11-04.md), [benchmark runbook](CLI-BENCHMARK-RUNBOOK.md) and CI artifacts. These have documented limitations and are preferable for comparison.

## Adopted operating policy proposed by this PR

1. **Never commit raw `perf.data`, `gmon.out`, `*.profraw`, unredacted `gprof` text or ad hoc local benchmark output.** Treat locally generated profiles as private artifacts because they can contain paths, process names and other operational details.
2. Keep generated profiles and benchmark CSV/JSON in ignored local output paths or GitHub CI artifacts with explicit retention and access policy. Do not put real customer filenames or data in fixtures.
3. Preserve **sanitized** summary reports only when they record exact Git SHA, compiler/build flags, CPU/OS/kernel, filesystem, storage, workload construction, file sizes/count, timing method, samples and resource limits.
4. Do not call an execution safe merely because it uses disposable fixtures: inspect harness-generated paths and cleanup logic, and avoid following symlinks or deleting paths not created by that invocation.
5. When saving a security-relevant failure profile, use restricted-access retention and a documented disclosure/cleanup process rather than publishing binary process traces.
6. For public performance claims, use repeatable median/tail results and environment data; never convert a historical gprof percentage into a modern speed guarantee.
7. **Owner action required:** inspect the binary `perf.data` privately, decide whether the two tracked historical files have a legitimate audit/archival value, and approve index cleanup in a **separate small PR**. Removing them from current Git tracking would not erase previous commits. History rewriting, if ever needed for sensitive information, requires a distinct security/legal review.

## Why no tracked-file deletion in HYGIENE-01

The current branch intentionally does **not** delete or rewrite either tracked artifact. The existing milestone explicitly requires owner review before deletion. The GitHub connector exposes the blob's size but not enough safely decoded content to determine what the binary contains. Preserve evidence and report uncertainty instead of erasing or claiming it is harmless.

## Native scanner evidence

The manual-only native scanner workflow retains only successful JSON reports and selected hosted-runner provenance for 30 days. It uses generated disposable inputs, exact Git SHA and binary SHA-256, bounded inputs and timeout. It does not upload raw compile commands, profiles or user data; runner measurements do not establish target-host performance or release readiness. No dispatch is implied by adding the workflow.

## Technical follow-up

- Add appropriate ignored local profiling and benchmark paths (this PR), including non-root build locations where relevant.
- Separately evaluate `make profile`, `make perf-record`, `make gprof` and `make clean` for generated-artifact location, least privilege (`sudo perf` should not be a default developer dependency), no-pager CI behavior and ownership-safe cleanup.
- Benchmark source and runner metadata should use the established performance CI, not old checked-in profiles.
- Keep `HYGIENE-02` (owner-approved tracked-artifact removal) blocked until reviewed; do not automatically run `git rm`, `git filter-repo` or force-push.

## Evidence and limitations

This is a remote-source and documentation audit; **no** profiling commands, CI builds, root commands, file deletions, sanitizers or user-data operations were run locally. A passing docs PR CI does not certify the contents of `perf.data` or justify deleting it.
