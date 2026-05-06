# ============================================================
# Makefile — thin wrapper around CMake for Fedora 44
# Usage:
#   make              → Debug build
#   make release      → Release build (O3 + LTO)
#   make profile      → Profile build (perf/gprof)
#   make run          → Debug build + run
#   make perf         → Profile build + perf stat
#   make clean        → Remove build dirs
#   make asm          → Show generated assembly
# ============================================================

BUILD_DIR_DEBUG   := build/debug
BUILD_DIR_RELEASE := build/release
BUILD_DIR_PROFILE := build/profile
TARGET            := bin/file-cleaner
GENERATED_FILES   := ./generated_files/

.PHONY: all debug release profile run perf gprof clean asm help

all: debug

debug:
	@mkdir -p $(BUILD_DIR_DEBUG)
	@cmake -S . -B $(BUILD_DIR_DEBUG) \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-G Ninja
	@cmake --build $(BUILD_DIR_DEBUG) --parallel $$(nproc)
	@echo "✅  Debug build → $(TARGET)"

release:
	@mkdir -p $(BUILD_DIR_RELEASE)
	@cmake -S . -B $(BUILD_DIR_RELEASE) \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-G Ninja
	@cmake --build $(BUILD_DIR_RELEASE) --parallel $$(nproc)
	@echo "✅  Release build → $(TARGET)"

profile:
	@mkdir -p $(BUILD_DIR_PROFILE)
	@cmake -S . -B $(BUILD_DIR_PROFILE) \
		-DCMAKE_BUILD_TYPE=Profile \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-G Ninja
	@cmake --build $(BUILD_DIR_PROFILE) --parallel $$(nproc)
	@echo "✅  Profile build → $(TARGET)"

run: debug
	@./$(TARGET) $(GENERATED_FILES)

perf: profile
	@echo "🔍 Running perf stat..."
	@sudo perf stat -e cycles,instructions,cache-misses,branch-misses \
		./$(TARGET) $(GENERATED_FILES)

perf-record: profile
	@echo "🔍 Recording perf data..."
	@sudo perf record -g ./$(TARGET) $(GENERATED_FILES)
	@sudo perf report

gprof: profile
	@./$(TARGET) $(GENERATED_FILES)
	@gprof ./$(TARGET) gmon.out | less

asm:
	@mkdir -p build/asm
	@$(CC) $(CFLAGS) -O2 -S -masm=intel \
		$$(find src -name '*.c') \
		-I include
	@mv *.s build/asm/ 2>/dev/null || true
	@echo "✅  Assembly files → build/asm/"

clean:
	@rm -rf build/ bin/ lib/
	@rm -rf Jpeg/ Png/ Others
	@echo "🧹 Cleaned build artifacts"

help:
	@echo ""
	@echo "  make           → Debug build (ASan + UBSan)"
	@echo "  make release   → Release build (O3 + LTO + march=native)"
	@echo "  make profile   → Profile build (perf/gprof ready)"
	@echo "  make run       → Build + run"
	@echo "  make perf      → Build + perf stat"
	@echo "  make perf-record → Build + perf record + report"
	@echo "  make gprof     → Build + gprof report"
	@echo "  make asm       → Dump Intel-syntax assembly"
	@echo "  make clean     → Remove all build files"
	@echo ""
