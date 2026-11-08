# Thin wrapper around CMake. Build products stay under build/.
# Safe defaults: Debug build, dry-run execution, bounded parallelism.
# Real file operations require explicit confirmation variables.

BUILD_DIR_DEBUG   := build/debug
BUILD_DIR_RELEASE := build/release
BUILD_DIR_PROFILE := build/profile
TARGET_DEBUG      := $(BUILD_DIR_DEBUG)/file-cleaner
TARGET_RELEASE    := $(BUILD_DIR_RELEASE)/file-cleaner
TARGET_PROFILE    := $(BUILD_DIR_PROFILE)/file-cleaner

GENERATED_FILES   ?= ./generated_files/
JOBS              ?= 2
BENCH_FILES       ?= 10000
BENCH_RUNS        ?= 30
BENCH_WORKERS     ?= 2
BENCH_OUTPUT      ?= benchmark-results/cli-dry-run
MOVE_BENCH_FILES  ?= 1000
MOVE_BENCH_RUNS   ?= 10
MOVE_BENCH_WORKERS ?= 2
MOVE_BENCH_BYTES  ?= 1024
MOVE_BENCH_OUTPUT ?= benchmark-results/cli-moves

# Restrict build parallelism to a predictable, reviewable range.
ifneq ($(filter-out 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16,$(JOBS)),)
$(error JOBS must be an integer from 1 through 16)
endif

# Quote caller-overridable path/argument values for POSIX shells.
shquote = '$(subst ','"'"',$(1))'

.PHONY: all debug release profile test validate run run-execute perf perf-record gprof benchmark benchmark-moves clean asm help

all: debug

debug:
	@cmake -S . -B "$(BUILD_DIR_DEBUG)" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja
	@cmake --build "$(BUILD_DIR_DEBUG)" --parallel "$(JOBS)"
	@echo "Debug build -> $(TARGET_DEBUG)"

release:
	@cmake -S . -B "$(BUILD_DIR_RELEASE)" -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja
	@cmake --build "$(BUILD_DIR_RELEASE)" --parallel "$(JOBS)"
	@echo "Release build -> $(TARGET_RELEASE)"

profile:
	@cmake -S . -B "$(BUILD_DIR_PROFILE)" -DCMAKE_BUILD_TYPE=Profile -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja
	@cmake --build "$(BUILD_DIR_PROFILE)" --parallel "$(JOBS)"
	@echo "Profile build -> $(TARGET_PROFILE)"

# Default local test path: Debug build includes ASan/UBSan and CTest tests.
test: debug
	@ctest --test-dir "$(BUILD_DIR_DEBUG)" --output-on-failure

# Full language-compatibility gate. Each standard gets an isolated build tree.
# Runs serially to keep memory use predictable on small Codespaces.
validate:
	@set -eu; \
	for standard in 11 17 23; do \
	  build_dir="build/validate-c$$standard"; \
	  echo "=== Validate C$$standard (Debug + ASan/UBSan) ==="; \
	  cmake -S . -B "$$build_dir" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DFC_C_STANDARD="$$standard"; \
	  cmake --build "$$build_dir" --parallel "$(JOBS)"; \
	  ctest --test-dir "$$build_dir" --output-on-failure; \
	done

run: debug
	@./"$(TARGET_DEBUG)" $(call shquote,$(GENERATED_FILES)) --dry-run

# This target can move files. It is disabled unless the caller opts in explicitly.
ifeq ($(CONFIRM_MOVE),YES)
run-execute: debug
	@./"$(TARGET_DEBUG)" $(call shquote,$(GENERATED_FILES))
else
run-execute:
	@echo "Refusing real file operations. Re-run with CONFIRM_MOVE=YES after reviewing the input folder."; exit 2
endif

perf: profile
	@echo "Profiling dry-run path; this does not move files."
	@sudo perf stat -e cycles,instructions,cache-misses,branch-misses ./ "$(TARGET_PROFILE)" $(call shquote,$(GENERATED_FILES)) --dry-run

perf-record: profile
	@echo "Recording dry-run profile."
	@sudo perf record -g ./ "$(TARGET_PROFILE)" $(call shquote,$(GENERATED_FILES)) --dry-run
	@sudo perf report

gprof: profile
	@./"$(TARGET_PROFILE)" $(call shquote,$(GENERATED_FILES)) --dry-run
	@gprof "$(TARGET_PROFILE)" gmon.out | less

benchmark: release
	@python3 tests/performance/benchmark_cli.py --binary "$(TARGET_RELEASE)" --config configs/default.conf --files "$(BENCH_FILES)" --runs "$(BENCH_RUNS)" --workers "$(BENCH_WORKERS)" --output $(call shquote,$(BENCH_OUTPUT))

# The benchmark harness creates isolated temporary fixtures; inspect its report.
# It performs actual file moves inside those fixtures, never on GENERATED_FILES.
benchmark-moves: release
	@python3 tests/performance/benchmark_moves.py --binary "$(TARGET_RELEASE)" --config configs/default.conf --files "$(MOVE_BENCH_FILES)" --bytes-per-file "$(MOVE_BENCH_BYTES)" --runs "$(MOVE_BENCH_RUNS)" --workers "$(MOVE_BENCH_WORKERS)" --output $(call shquote,$(MOVE_BENCH_OUTPUT))

asm:
	@mkdir -p build/asm
	@$(CC) $(CFLAGS) -O2 -S -masm=intel $$(find src -name '*.c' -type f -print) -I include
	@find . -maxdepth 1 -type f -name '*.s' -exec mv -- {} build/asm/ \;
	@echo "Assembly files -> build/asm/"

clean:
	@rm -rf -- build/ bin/ lib/
	@rm -f -- gmon.out perf.data
	@echo "Cleaned build artifacts; user data folders were left untouched."

help:
	@echo "make                 -> Debug build (ASan + UBSan)"
	@echo "make test            -> Debug build + CTest suite"
	@echo "make validate        -> C11, C17, C23 Debug build + tests"
	@echo "make release         -> Portable optimized Release build"
	@echo "make profile         -> Profiling build"
	@echo "make run             -> Debug dry-run (safe default)"
	@echo "make run-execute CONFIRM_MOVE=YES -> Explicit real file operations"
	@echo "make perf            -> Profile dry-run"
	@echo "make perf-record     -> Record dry-run profile"
	@echo "make gprof           -> Generate dry-run gprof report"
	@echo "make benchmark       -> Safe repeatable CLI dry-run benchmark"
	@echo "make benchmark-moves -> Real moves inside isolated temporary fixtures"
	@echo "make asm             -> Dump compiler-generated Intel-syntax assembly"
	@echo "make clean           -> Remove build artifacts only"
	@echo "JOBS=2 is the conservative default; override only for available resources."
