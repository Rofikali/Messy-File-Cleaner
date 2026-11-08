# Static Analysis Baseline

## Purpose

Use independent compiler and static-analysis tools to discover defects before
changing security-sensitive filesystem code. The first run is a discovery
baseline, not a release gate.

## Tools

The `static-analysis.yml` workflow runs on Ubuntu 24.04 with:

- Clang: independent compiler diagnostics and a full Debug build.
- clang-tidy: Clang diagnostic and static-analysis checks over production C
  translation units.
- cppcheck: warning, performance, and portability checks using CMake's
  compilation database, including configured test targets.

The workflow uploads text reports as the `static-analysis-reports` artifact
for 14 days. The initial clang-tidy and cppcheck steps are report-only. This is
intentional: existing findings must be reviewed and classified before a
reliable zero-warning policy can be adopted.

## Triage rules

Every finding should be classified as one of:

1. Confirmed defect: reproduce or demonstrate the unsafe behavior, then add a
   regression test and fix.
2. Likely defect: investigate with a focused test or code review before fix.
3. False positive: document why the analyzer's assumptions do not apply.
4. Accepted limitation: document the platform/compiler condition and owner.
5. Style or maintainability: schedule separately from correctness/security work.

Do not silence a finding globally. Prefer a narrow source-level suppression
only after documenting the reason and tool version. Do not bulk-apply
clang-tidy fixes to filesystem, path-validation, journal, or concurrency code.

## Promotion to gating

Do not turn analyzer output into a hard CI gate until:

- Reports from the baseline have been reviewed.
- Confirmed defects have been fixed and regression-tested.
- Remaining accepted findings have a documented rationale.
- The tools and checks are pinned or versioned sufficiently to avoid noisy
  behavior changes.
- The workflow is repeatable on the supported CI runner.

The existing C11/C17/C23 Debug/Release matrix remains the primary language
standard and test gate. This workflow adds independent analysis; it does not
replace the build or test matrix.
