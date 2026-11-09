# Messy File Cleaner

A Linux/POSIX file-organization tool written in **pure C**. It scans a directory, classifies files using configurable extension rules, and processes work through a bounded worker pipeline. File moves are protected by a journal-backed transaction flow so interrupted operations can be examined and recovered conservatively.

> **Project status:** Active development. Treat this repository as an engineering project under validation, not as a claim of production-grade or universally crash-proof file operations. Review the current tests, platform assumptions, and known limitations before using it on valuable data.

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

- **Language:** pure C. C++ is not part of this project.
- **Production baseline:** C17.
- **Compatibility validation:** C11 and C23 are tested as separate build targets; C23 is not the minimum production requirement.
- **Structure:** modular monolith with narrow module contracts.
- **Concurrency:** bounded queues and a worker pool; preserve each queue's documented producer/consumer topology.
- **Filesystem operations:** use explicit error handling and conservative recovery. Treat paths and filesystem entries as untrusted.
- **Testing:** unit, integration, end-to-end, fault-injection, sanitizer, container, and static-analysis checks where configured.
- **Performance:** measure before optimizing. Filesystem throughput depends on storage, filesystem metadata, cache state, file sizes, and worker count.

Do not add a broker, database, microservices, C++, Rust, or assembly merely to make the architecture look more advanced. Add a new technology only when a concrete requirement and an engineering review justify it. Assembly, if ever needed, must be isolated behind a C interface, supported by a portable C fallback, and justified by repeatable profiling.

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

FCJ4 is a separate, future journal protocol under staged development. The current repository contains a pure in-memory parser/replay implementation and an isolated canonical serializer/writer-state module used by tests. These modules are not the production journal, are not exposed through the CLI, and do not read or write `journal/move.log` or a `journal/move.fcj4` file.

The current FCJ4 writer-state implementation work adds owned BEGIN/SEALED/ACTION_DEF evidence, pre-prepare cross-record checks, replay-seeded writer-state import with deep-copied strings, exact MOVE/ACTION sequence restoration, and record-limit accounting. Focused tests cover replay lifetime independence and selected evidence mismatches; the full adversarial test matrix and complete CI validation remain required before this milestone is accepted. It does **not** provide durable append, locking, `fsync`, recovery planning, or filesystem mutation. A successful in-memory commit is not proof that bytes were durably written.

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

## Contributing

Read [AGENTS.md](AGENTS.md) and [SKILLS.md](SKILLS.md) before changing the architecture or implementation.

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
