# Messy File Cleaner

> **Configuration input contract:** Physical config lines must reject embedded NUL bytes before string parsing; failed loads preserve caller state; loader requires a nonempty path and initialized power-of-two map capacity with size not exceeding capacity; extension names reject control bytes; rejected extension keys and destinations must not be echoed into diagnostics. Hash-map initialization and access reject impossible byte extent or size metadata; callers still own backing storage. Classification uses the last POSIX path component; leading-dot-only and trailing-dot names use the fallback. SPSC queue initialization is exclusive; its unit suite checks concurrent FIFO transfer across repeated wraparound. Worker move diagnostics escape nonprintable/non-ASCII bytes and backslashes under a stream lock; original filesystem paths are unchanged. Root/output validation reports fixed rejection reasons without echoing untrusted paths. Linux watch registration requires a directory and rejects final-component symlinks; ancestor path identity is not established. Shared dispatch rejects empty paths and worker counts outside 1..MAX_WORKERS. Watcher event headers are copied before decoding; actionable names must be nonempty basename components. Worker notification and shutdown reject out-of-range pool metadata before indexing fixed arrays. The process-wide worker storage has one owner; duplicate starts and wrong-context lifecycle operations are rejected under caller serialization. Watcher idle-poll metrics count actual poll timeouts; integration waits for observed readiness instead of a fixed startup sleep. Scanner cancellation is checked before root access and immediately before callback handoff; it remains cooperative. Scanner errors retain the first observed failure even when later cancellation or callback failure stops traversal. Linux scanner strips trailing root separators before no-follow opening, preserving the root slash; ancestor identity is not established. Linux watcher also trims trailing root separators before no-follow registration and builds event paths from the trimmed root. Scanner and watcher share the internal root-path copy helper; it trims only trailing slashes and performs no filesystem resolution. CLI root preflight uses the same lexical root-copy helper before its existing lstat/realpath checks. CLI output preflight, scanner and watcher use a shared checked lexical path-join helper; callers retain component validation and filesystem policy. Scanner path construction and frame growth failures stop discovery and unwind owned frames; deep-tree allocation fault regressions check descriptor balance. Scanner directory acquisition stops on EMFILE, ENFILE or ENOMEM and unwinds owned frames; deterministic child-open fault tests check attempts and descriptor balance. Scanner resource regressions cover directory-stream acquisition faults and a real child-process RLIMIT_NOFILE exhaustion test without changing the parent or system limit. BENCH-SCALE-01 adds a bounded native C scanner benchmark with callback counts, monotonic timing, CPU and process peak RSS; smoke checks do not establish large-scale capacity. Native scanner evidence has a bounded manual-only C17 Release workflow with source/binary and runner provenance; no automatic scale run or spending authorization. BENCH-SCALE-01 final budget slot adds semantic evidence validation before upload; no performance result or release acceptance is inferred. Malformed bare recognized configuration settings use the same C whitespace classification as key normalization, including vertical tab and form feed; rejection preserves caller configuration. Hash-map existing-key updates accept overlapping borrowed value strings, including a suffix, without undefined memcpy overlap. Classifier initialization ignores a NULL context; classification rejects NULL contexts and NULL/empty filenames without changing valid-path routing. SPSC queue APIs reject NULL queue/event arguments without transferring events; NULL initialization is a no-op and NULL emptiness queries return true. Valid initialized SPSC ordering and capacity are unchanged. Workers reject empty or unterminated event paths and NULL/empty classification results before diagnostics or moving; invalid work increments failures and stops intake with worker_error while existing queued work drains. Scanner entry and opened-directory metadata resource exhaustion terminates discovery and unwinds owned frames; entry and opened-directory lookup fault regressions verify traversal termination and descriptor balance, including nested failure and successful rescan. See [live board](next.md) for implementation and verification status. FS-01B/REL-01 remain blocked.

> **Current owner contract (24 Nov 2026):** Every authorized contributor (human, ChatGPT, Codex, Claude Code or other agent) uses the same resume-first [handoff contract](docs/AUTOPILOT_EXECUTION_CONTINUATION.md), [live board](next.md) and [feature ledger](docs/feature-budget.json). Hourly and manual `go ahead` are additive. Reconcile all authors' PRs, finish existing eligible work before starting another, count all opened PRs toward one stable five-PR budget, and update affected docs in the same PR. Actual GitHub state supersedes historical milestones below. No fake approval, security bypass or release authority.

> **Execution recovery (23 Nov 2026):** use the [hourly continuation and stall recovery](docs/AUTOPILOT_EXECUTION_CONTINUATION.md) for checkpoints, resume-first task selection, two-no-change escalation, and proportional L1–L7/CISSP/CCIE/Linux/SRE/CA-MBA review. This is a repository procedure, **not a running scheduler**; current GitHub and actual ChatGPT task history supersede dated snapshots.

> **Autopilot governance (23 Nov 2026):** owner allows safe reviewed-by-agent PR merges with **zero mandatory human approvals**, but not bypassing CI, protection or FS-01B/release restrictions. [Policy and post-merge docs workflow](docs/AUTOPILOT_ZERO_REVIEW_AND_DOC_SYNC.md) · [GitHub importable main ruleset](.github/rulesets/autopilot-main-no-human-review.json). Committing JSON does not activate the ruleset.

> **Configuration read-error safety (23 Nov 2026):** CONFIG-READ-ERROR-01 rejects stream read/close failures instead of accepting partial configuration; disposable Linux regression, pending exact-head CI. No destructive operations or release.

> **Configuration validation (23 Nov 2026):** scoped CONFIG-EXT-KEY-01 work rejects missing extension names (`ext.=` / `ext..=`), with disposable negative tests. PR-head CI and merge status must be checked live; this does not authorize destructive source retirement or release.

A Linux/POSIX file-organization tool with a **pure-C production engine** and isolated Rust **test-support** tooling. It scans a directory, classifies files using configurable extension rules, and processes work through a bounded worker pipeline. File moves are protected by a journal-backed transaction flow so interrupted operations can be examined and recovered conservatively.

> **Project status:** Active development; not certified for production or valuable unbacked-up data. The [FS-01B test](docs/quality/FS_01B_RETIREMENT_RACE_CHARACTERIZATION.md) reproduced a **P0 source-retirement check/use risk**: a substituted name can be unlinked after a prior inode check. The [proposed FS-01B-GUARD](docs/security/FS_01B_SHARED_SOURCE_GUARD.md) refuses moves from group/other-writable or non-owned input directories to reduce other-UID exposure, but **does not fix same-UID interference, arbitrary malicious writers, or power-loss recovery**. If merged, previously accepted shared-writable directories will receive `EPERM` instead of being moved. Require verified backups before any real move.

## Feature progression and verification (23 November 2026)

[PR #148](https://github.com/Rofikali/Messy-File-Cleaner/pull/148) merged at `3b32efbe8fad2047f3588fa51c907525a4e4dca4`, rejecting overlong physical configuration lines. Its tested PR head `36383fd2ca31cba9489a7e88b530b29b1fd6298d` passed CMake, Docker, Makefile and Ubuntu 24.04 workflow families. PR #150 subsequently merged trailing-operand rejection for `ext.*` mappings at `74c0639e9656eb37339e96cdc9ed7d312682b3f0`; a new safe feature requires live selection and ledger reconciliation; see [live board](next.md) and [feature budget](docs/feature-budget.json). These CI results do not establish manual Ubuntu/Docker Desktop/Compose, real-filesystem, backup or power-loss acceptance. FS-01B P0 and REL-01 remain blocked; no release is authorized.

## Opt-in dry-run JSON summary (PR #145, merged; aggregate-only)

Proposed usage: `file-cleaner --root /path/to/disposable-input --dry-run --once --report-json`. This emits one JSON line on standard output with `schema_version: 1`, `kind: dry_run_summary`, `complete`, `events_seen`, `events_enqueued`, `previewed_moves`, and `move_failures`. Existing human-readable output remains; consumers must select the JSON line, not parse all stdout as JSON. No filenames or paths appear in the JSON record. This is an aggregate diagnostic, **not** a deterministic per-file operation plan, collision/conflict inventory, or permission to execute. `--report-json` without dry-run or with watch mode is rejected. Feature is **merged but only aggregate-level**, not released; all FS-01B and REL-01 restrictions remain.

## Project roadmap, current status, and engineering docs

- **How documentation stays current with each code/CI milestone:** [milestone documentation transaction](docs/MILESTONE_DOCUMENTATION_WORKFLOW.md), [PR checklist](.github/pull_request_template.md), and [historical archive](next-history.md). Existing PR and CI state must be verified live; the compact board is a dated checkpoint, not a GitHub API feed.
- **What is NOW / NEXT / LATER:** [`next.md`](next.md) — live board of actionable milestones, gates and precise PR/CI evidence; older snapshots are retained under a clearly marked historical section.
- **What advanced features might come after current milestones:** [Advanced C17 filesystem/SDK/desktop roadmap](docs/ADVANCED_FEATURE_ROADMAP.md) and [security, verification, seven-level engineering and business acceptance gates](docs/ADVANCED_FEATURE_ACCEPTANCE.md). These do not certify unbuilt capabilities.
- **What is finished, half implemented, started, test-only, blocked or not started:** [Component completion and status inventory](docs/PROJECT_COMPLETION_AND_STATUS_INVENTORY.md).
- **Where to find authoritative docs:** [Documentation map](docs/README.md).
- **How seven engineering levels, security, CTO, CA/MBA and management review changes:** [Operating model](docs/ENGINEERING_OPERATING_MODEL.md).
- **CI/CD and supported Make targets:** [CI/CD and Makefile guide](docs/CI_CD_AND_MAKEFILE_OPERATIONS.md).
- **How an agent resumes and selects the next small PR:** [Continuation/handoff runbook](docs/AGENT_CONTINUATION_AND_HANDOFF.md).
- **Product direction and free/paid/SDK hypotheses:** [Product and business strategy](docs/PRODUCT_AND_BUSINESS_STRATEGY.md).

**Technology status:** The shipped application remains **C17**; C11 and C23 are compatibility-validation targets. PRs #81 and #82 approved/merged **isolated Rust test-support** for future parser verification. That Rust crate does not call C yet and is not part of the executable. No public `.a`/`.so` SDK, production Rust FFI, FCJ4 production recovery, SaaS/API, GUI, pricing, or automatic deployment is implemented by this documentation.

## Engineering direction

The project is intentionally a **modular C application (modular monolith)**, not a microservices system.

Its architecture combines:

- **Pipeline / producer-consumer processing** for scanning, dispatching, classification, queues, and worker execution.
- **A journal-backed state machine** for tracking file-move progress and supporting recovery after interruption.
- **Event-style worker notifications** to wake workers and coordinate shutdown.
- **Explicit filesystem boundaries** so risky operations can be tested with deterministic failure injection.

This is event-driven in selected places, but it is **not a full distributed Event-Driven Architecture (EDA)**. There is no message broker, distributed event bus, or eventual-consistency layer. Those would add operational complexity without a demonstrated need for this local file-management application.

### Journal states are transaction records, not a message-broker event stream

The journal records the durable progress of a move. State names describe the operation's recorded state:

| Record | Meaning in the move/recovery protocol |
|---|---|
| `STARTED` | The operation has been recorded as begun. |
| `PUBLISHED` | The destination has been published. |
| `COMPLETED` | The move operation has reached its completed state. |
| `FAILED` | The operation has recorded a failure outcome. |
| `RECOVERED` | Recovery has recorded that it handled the interrupted operation. |

The journal is evidence used by recovery; it is not merely a log for display. Recovery must follow the **last complete, valid record** and inspect filesystem state conservatively. A failed or missing transition must never be treated as if it had succeeded.

Important safety rules:

- Do not overwrite an existing destination.
- Do not retire the source before the destination has been safely copied and published.
- If an error occurs after publication, retain the complete destination; do not perform a destructive rollback just to make the operation appear atomic.
- If the journal contains a partial or ambiguous tail, fail closed rather than guessing or silently truncating evidence.
- Recovery should be repeatable where the recorded state permits it. An uncertain outcome must remain explicit.

The state names are implementation-level protocol details. If the protocol changes, update the journal format, recovery rules, tests, and documentation together.

## Current design map

| Area | Responsibility |
|---|---|
| CLI / application context | Parse options, validate roots and outputs, initialize and shut down the application |
| Configuration | Load worker settings and extension-to-folder rules |
| Scanner | Discover filesystem entries under the selected root |
| Classifier | Map files to configured destination categories |
| Queue / worker pool | Dispatch work, apply backpressure, notify workers, and coordinate shutdown |
| Mover | Copy and publish files while preserving data on failure |
| Journal | Record move progress and drive conservative recovery |
| Watcher | Feed filesystem changes into the processing pipeline in watch mode |

The production source list is explicit in `CMakeLists.txt`. Placeholder source files are not proof that a feature is implemented and are not automatically included in the executable.

## Architecture decisions

- **Production language:** pure C; an accepted Rust test-only pilot exists outside production. C++ is not part of this project.
- **Production baseline:** C17.
- **Compatibility validation:** C11 and C23 are tested as separate build targets; C23 is not the minimum production requirement.
- **Structure:** modular monolith with narrow module contracts.
- **Concurrency:** bounded queues and a worker pool; preserve each queue's documented producer/consumer topology.
- **Filesystem operations:** use explicit error handling and conservative recovery. Treat paths and filesystem entries as untrusted.
- **Testing:** unit, integration, end-to-end, fault-injection, sanitizer, container, and static-analysis checks where configured.
- **Performance:** measure before optimizing. Filesystem throughput depends on storage, filesystem metadata, cache state, file sizes, and worker count.

Do not add a broker, database, microservices, C++, **production Rust**, or assembly merely to make the architecture look more advanced. The accepted, isolated Rust test-support pilot remains separate from the C production executable. Add a new technology only when a concrete requirement and an engineering review justify it. Assembly, if ever needed, must be isolated behind a C interface, supported by a portable C fallback, and justified by repeatable profiling.

## Requirements

The canonical development environment is Ubuntu 24.04 LTS.

You need:

- GCC or Clang with C11/C17/C23 support for the selected validation target
- CMake 3.21 or newer
- POSIX/Linux development environment
- pthreads
- Python 3 for the performance/fixture test helpers

Docker builds use a multi-stage Ubuntu 24.04 image.

## Build

Clone the repository and build with CMake:

```sh
git clone https://github.com/Rofikali/Messy-File-Cleaner.git
cd Messy-File-Cleaner

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DFC_C_STANDARD=17
cmake --build build --parallel
```

For a debug build with the configured AddressSanitizer and UndefinedBehaviorSanitizer flags:

```sh
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DFC_C_STANDARD=17
cmake --build build-debug --parallel
```

To validate the language-standard matrix locally, configure separate build directories:

```sh
for standard in 11 17 23; do
  cmake -S . -B "build-c${standard}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=ON \
    -DFC_C_STANDARD="${standard}"
  cmake --build "build-c${standard}" --parallel
  ctest --test-dir "build-c${standard}" --output-on-failure
done
```

Run the test suite:

```sh
ctest --test-dir build --output-on-failure
```

## Run

The CLI accepts either a positional root or `--root`:

```sh
./build/file-cleaner /path/to/input --dry-run --once
```

Useful options:

| Option | Purpose |
|---|---|
| `<root>` or `--root <root>` | Directory to scan |
| `--config <file>` | Configuration file (defaults to `configs/default.conf`) |
| `--dry-run` | Process and report planned moves without opening the move journal or moving files |
| `--once` | Run one batch scan |
| `--watch` | Run in watcher mode |
| `--workers <n>` | Override the configured worker count |
| `--force` | Explicitly bypass the dangerous-root guard; it does **not** bypass path and filesystem safety validation |

When using Docker Compose, the default `compose.yaml` also mounts `/input` read-only; real moves require the separate **explicit** `compose.execute.yaml` overlay described in [the Docker runbook](docs/docker/DOCKER-RUNBOOK.md). Merely overriding the default command does not make the input writable.

Start with `--dry-run` on a disposable test directory. Inspect the output and configuration before running a real move. Do not point the tool at system directories, home directories, mounted data, or valuable files unless you understand the consequences and have a verified backup.

### Configuration

The default configuration is `configs/default.conf`. It currently selects batch mode, configures worker settings, and maps extensions to destination folders. For example:

```ini
mode=batch
worker_threads=2
batch_size=64
idle_sleep_ms=1

ext.jpg=Jpeg/
ext.jpeg=Jpeg/
ext.png=Png/
ext.html=Html/
```

Use a separate test configuration and disposable input/output directories when experimenting with new rules.

## FCJ4 journal development status

FCJ4 is a separate journal protocol under staged development. The repository contains a pure in-memory parser/replay implementation, an isolated canonical serializer/writer-state module, and a Linux-only test append adapter. These components are test-only, are not linked into the production executable, are not exposed through the CLI, and do not read or write `journal/move.log` or `journal/move.fcj4`.

The writer-state module owns BEGIN/SEALED/ACTION_DEF evidence, performs pre-prepare cross-record checks, imports replay state with deep-copied strings, restores exact MOVE/ACTION sequences, and enforces record-limit accounting. The separate test adapter operates only on a caller-supplied descriptor for a disposable private temporary file. It uses explicit-offset `pwrite()`, rejects `O_APPEND`, takes a non-blocking advisory `flock()`, checks file identity/size/permissions, and requires successful `fsync()` followed by an explicit in-memory writer-commit acknowledgement before another append. Fault-injection tests cover short writes, EINTR, partial-write and fsync uncertainty, commit reconciliation, cleanup failures, and process-exit replay.

This adapter does **not** open or create the application journal path, and it does not provide production durability, recovery planning, or filesystem mutation. Process-exit tests are not power-loss tests. A successful in-memory commit alone is not proof that bytes were durably written.

A valid FCJ4 replay proves only that the supplied byte sequence satisfies the parser's structural and transition rules. It does not prove journal authenticity, current filesystem state, or permission to execute recovery. Do not manually create, edit, or use FCJ4 records to recover files.

FCJ2/legacy production journal behavior remains separate. FCJ3 remains diagnostic/test-only and must not be mixed into the production FCJ2/legacy journal.

## Data safety and recovery

File movement is more complicated than copying bytes. It involves temporary files, publication, directory metadata, synchronization, source retirement, and journal updates. A process can fail between any two of these steps.

The implementation and tests therefore distinguish failures before publication from failures after publication and from failures during source retirement. In particular, a failure after a destination has been published can leave both source and destination present; this is safer than deleting the only known-good copy.

The journal is stored at the configured application-relative journal location (`journal/move.log` in the current implementation). Keep it with the application state and do not manually edit or delete it while operations may be in progress. If recovery reports an error, stop and preserve the source, destination, and journal for investigation.

No README can guarantee safety on every filesystem, kernel, mount configuration, storage device, or power-loss scenario. Durability depends on correct system-call ordering and the guarantees of the underlying platform and hardware.

## Validation and quality

The repository includes tests for configuration/path safety, scanning, queue behavior, worker lifecycle, watcher integration, moving, journaling/recovery, CLI end-to-end behavior, and benchmark smoke paths.

The quality workflow is intended to validate:

- C11, C17, and C23 builds in Debug and Release configurations
- Docker build and runtime smoke behavior
- Ubuntu 24.04 build/test behavior
- Static analysis and security-relevant failure paths
- Deterministic filesystem and journal failure injection

A passing test suite is evidence for the tested cases, not proof that every crash, filesystem, or hardware failure is handled. Read the CI results for the exact commit before relying on a validation claim.

## Performance philosophy

Correctness and data preservation come before speed.

Benchmark representative workloads: many small files, large files, deep directory trees, many directories, warm and cold cache cases, and cross-filesystem moves. Report the environment, worker count, filesystem, storage, compiler, build flags, number of repetitions, median and tail latency, throughput, and memory use.

Do not describe end-to-end file moves as nanosecond operations. Nanosecond-scale measurements may be relevant to small CPU-bound operations, but disk and filesystem work must be measured in realistic units and distributions.

## Security principles

- Treat paths, configuration values, filenames, and watcher events as untrusted input.
- Validate the input root and all output paths; reject dangerous overlap and symlink cases.
- Use least privilege. Do not run the container as root.
- Do not mount the Docker socket or host root into the container.
- Never truncate or overwrite an existing user destination.
- Check system-call results and preserve useful error information.
- Keep test fault-injection controls out of production builds.
- Never commit credentials, private user files, runtime journals, or benchmark artifacts.

## CI and release status

The project has GitHub workflows for CMake Debug/Release and C11/C17/C23 tests, Makefile validation, Ubuntu 24.04 containers, Docker smoke, Rust test support and conditional FCJ4 TSan; manual large-scale/performance benchmark workflows are separate. **These are build/test workflows, not an automatic software deployment or production release.** Use [CI/CD and Makefile operations](docs/CI_CD_AND_MAKEFILE_OPERATIONS.md) for actual triggers and evidence lanes. The existing `Makefile` remains unchanged by the documentation-alignment milestone; proposed `status`, `self-check` and `rust-check` targets are **future reviewed changes**, not current commands.

## Contributing

Read [AGENTS.md](AGENTS.md) for binding rules and [skills.md](skills.md) for the contributor workflow; [SKILLS.md](SKILLS.md) is a non-binding engineering knowledge catalogue. Check the current [work register](next.md) and [documentation index](docs/README.md) before choosing a new milestone.

Every change should:

1. State the problem and expected behavior.
2. Define module ownership, pointer lifetime, thread-safety, error, and cancellation contracts where relevant.
3. Include focused tests, including failure-path tests for filesystem or journal changes.
4. Update documentation when behavior or protocol semantics change.
5. Run the relevant build, test, sanitizer, and static-analysis checks.
6. Report exactly what was run, what passed, and what remains unverified.

Keep changes small enough to review. Never weaken a safety invariant to make a benchmark or CI job pass.

## License

No license is declared in this README. Check the repository's license file and ownership terms before redistributing or incorporating this project.


Configuration validation milestone: `CONFIG-MODE-MISSING-01` rejects a bare `mode` key rather than silently accepting the batch default. This does not change valid mode values, filesystem preservation guarantees, or release readiness. FS-01B and REL-01 remain blocked.


Configuration validation: `CONFIG-EXT-MISSING-01` rejects missing extension destination mappings rather than silently ignoring routing configuration. No filesystem retirement or release authorization is implied.


> **CONFIG-ATOMIC-LOAD-01 (23 Nov 2026, proposed PR):** C17 configuration loading now stages scalar settings and caller-owned extension map entries, committing only after successful parse/read/close. Failed loads retain prior caller state; disposable late-failure regression added. This is not a release or filesystem-retirement safety approval. Exact-head CI and manual acceptance remain NOT RUN pending PR verification. FS-01B and REL-01 remain BLOCKED.

> **CONFIG-KEY-WIDTH-01 (24 Nov 2026, proposed PR):** Configuration parsing rejects keys of 64 bytes or more instead of silently ignoring a truncated key. This is a validation hardening change, not release approval; see `next.md` and `docs/feature-budget.json` for CI and milestone status.
