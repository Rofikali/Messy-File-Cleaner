# Makefile safety and developer workflow

The Makefile is the supported local developer entry point. It wraps CMake and
keeps build outputs under `build/`. CMake remains the build-system source of
truth; the Makefile provides predictable, documented commands.

## Safe default workflow

```sh
make help
make                 # Debug build; ASan + UBSan
make test            # Debug build + CTest
make validate        # C11, C17, C23 Debug builds and tests
make release         # Optimized portable Release build
```

Build parallelism defaults to two jobs to avoid exhausting memory/CPU in small
Codespaces. Override deliberately, for example `make test JOBS=4`, after
checking the available resources.

## File-operation safety

- `make run` always passes `--dry-run`.
- `make run-execute` is disabled unless `CONFIRM_MOVE=YES` is supplied.
  Review the input root and configuration before opting in.
- `make benchmark-moves` performs real moves only inside temporary fixtures
  created by its benchmark harness. It does not use `GENERATED_FILES`.
  Treat the harness as destructive to its own temporary data and do not point
  its output at valuable user data.
- `make clean` removes only `build/`, `bin/`, `lib/`, `gmon.out`, and
  `perf.data`. Do not store user files in those build/artifact locations.

## Profiling and assembly

- `make perf`, `make perf-record`, and `make gprof` execute the dry-run
  path. Linux permissions may require elevated privileges for `perf`.
- `make asm` asks the configured C compiler to emit Intel-syntax assembly
  from the C translation units. This is an inspection aid, not a production
  assembly implementation. The project remains pure C; generated assembly is
  compiler output and must not be checked in as a replacement for portable C.

## Engineering rules

- C17 is the production baseline. C11 and C23 are compatibility validation
  targets, not separate production implementations.
- Never use a benchmark result to justify weakening data-preservation rules.
- Run correctness and sanitizer tests before performance comparisons.
- Review every command's expanded inputs before using custom Make variables.
- Make targets do not replace code review, CI, or platform-specific validation.
