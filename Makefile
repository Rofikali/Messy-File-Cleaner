# Thin wrapper around CMake. Build products stay under build/.
# Use dry-run targets by default; run-execute is intentionally explicit.

BUILD_DIR_DEBUG   := build/debug
BUILD_DIR_RELEASE := build/release
BUILD_DIR_PROFILE := build/profile
TARGET_DEBUG      := $(BUILD_DIR_DEBUG)/file-cleaner
TARGET_RELEASE    := $(BUILD_DIR_RELEASE)/file-cleaner
TARGET_PROFILE    := $(BUILD_DIR_PROFILE)/file-cleaner
GENERATED_FILES   := ./generated_files/
BENCH_FILES       ?= 10000
BENCH_RUNS        ?= 30
BENCH_OUTPUT      ?= benchmark-results/cli-dry-run
MOVE_BENCH_FILES  ?= 1000
MOVE_BENCH_RUNS   ?= 10
MOVE_BENCH_BYTES  ?= 1024
MOVE_BENCH_OUTPUT ?= benchmark-results/cli-moves

.PHONY: all debug release profile run run-execute perf perf-record gprof benchmark benchmark-moves clean asm help

all: debug

debug:
	@cmake -S . -B $(BUILD_DIR_DEBUG) -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja
	@cmake --build $(BUILD_DIR_DEBUG) --parallel $$(nproc)
	@echo "Debug build -> $(TARGET_DEBUG)"

release:
	@cmake -S . -B $(BUILD_DIR_RELEASE) -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja
	@cmake --build $(BUILD_DIR_RELEASE) --parallel $$(nproc)
	@echo "Release build -> $(TARGET_RELEASE)"

profile:
	@cmake -S . -B $(BUILD_DIR_PROFILE) -DCMAKE_BUILD_TYPE=Profile -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja
	@cmake --build $(BUILD_DIR_PROFILE) --parallel $$(nproc)
	@echo "Profile build -> $(TARGET_PROFILE)"

run: debug
	@./$(TARGET_DEBUG) $(GENERATED_FILES) --dry-run

run-execute: debug
	@./$(TARGET_DEBUG) $(GENERATED_FILES)

perf: profile
	@echo "Profiling dry-run path; this does not move files."
	@sudo perf stat -e cycles,instructions,cache-misses,branch-misses ./$(TARGET_PROFILE) $(GENERATED_FILES) --dry-run

perf-record: profile
	@echo "Recording dry-run profile."
	@sudo perf record -g ./$(TARGET_PROFILE) $(GENERATED_FILES) --dry-run
	@sudo perf report

gprof: profile
	@./$(TARGET_PROFILE) $(GENERATED_FILES) --dry-run
	@gprof $(TARGET_PROFILE) gmon.out | less

benchmark: release
	@python3 tests/performance/benchmark_cli.py --binary $(TARGET_RELEASE) --config configs/default.conf --files $(BENCH_FILES) --runs $(BENCH_RUNS) --output $(BENCH_OUTPUT)

benchmark-moves: release
	@python3 tests/performance/benchmark_moves.py --binary $(TARGET_RELEASE) --config configs/default.conf --files $(MOVE_BENCH_FILES) --bytes-per-file $(MOVE_BENCH_BYTES) --runs $(MOVE_BENCH_RUNS) --output $(MOVE_BENCH_OUTPUT)

asm:
	@mkdir -p build/asm
	@$(CC) $(CFLAGS) -O2 -S -masm=intel $$(find src -name '*.c') -I include
	@mv *.s build/asm/ 2>/dev/null || true
	@echo "Assembly files -> build/asm/"

clean:
	@rm -rf build/ bin/ lib/
	@rm -f gmon.out perf.data
	@echo "Cleaned build artifacts; user data folders were left untouched."

help:
	@echo "make             -> Debug build (ASan + UBSan)"
	@echo "make release     -> Portable optimized Release build"
	@echo "make profile     -> Profiling build"
	@echo "make run         -> Debug dry-run (safe default)"
	@echo "make run-execute -> Explicitly move files in generated_files/"
	@echo "make perf        -> Profile dry-run"
	@echo "make perf-record -> Record dry-run profile"
	@echo "make gprof       -> Generate dry-run gprof report"
	@echo "make benchmark   -> Safe repeatable CLI dry-run benchmark"
	@echo "make benchmark-moves -> Real moves + journal benchmark on disposable fixtures"
	@echo "make asm         -> Dump Intel-syntax assembly"
	@echo "make clean       -> Remove build artifacts only"
