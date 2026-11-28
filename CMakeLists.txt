cmake_minimum_required(VERSION 3.21)

project(file-cleaner
  VERSION 1.0.0
  DESCRIPTION "Messy File Cleaner"
  LANGUAGES C
)

set(FC_C_STANDARD "17" CACHE STRING "C language standard (11, 17, or 23)")
set_property(CACHE FC_C_STANDARD PROPERTY STRINGS 11 17 23)
if(NOT FC_C_STANDARD MATCHES "^(11|17|23)$")
  message(FATAL_ERROR "FC_C_STANDARD must be one of: 11, 17, 23")
endif()

set(CMAKE_C_STANDARD ${FC_C_STANDARD})
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(CTest)
find_package(Threads REQUIRED)

# Keep the production source list explicit. Placeholder translation units are
# intentionally excluded until their module contracts have real implementations.
# Add each new production source here as part of the same reviewed change.
set(FC_SOURCES
  src/main.c
  src/classifier/classifier.c
  src/config/config.c
  src/core/lf_queue.c
  src/core/queue.c
  src/core/thread_pool.c
  src/journal/journal.c
  src/mover/mover.c
  src/scanner/scanner.c
  src/utils/fs_utils.c
  src/utils/hashmap.c
  src/watcher/watcher.c
)

add_executable(${PROJECT_NAME} ${FC_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  Threads::Threads
  m
)

target_compile_options(${PROJECT_NAME} PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Wformat=2
  -Wshadow
  -Wstrict-prototypes
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Build: Debug (AddressSanitizer + UndefinedBehaviorSanitizer)")
  target_compile_options(${PROJECT_NAME} PRIVATE
    -O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer
  )
  target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address,undefined)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "Build: portable Release (optimized, stack-protected)")
  target_compile_options(${PROJECT_NAME} PRIVATE
    -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2
  )
  target_link_options(${PROJECT_NAME} PRIVATE
    -Wl,-z,relro,-z,now
  )
elseif(CMAKE_BUILD_TYPE STREQUAL "Profile")
  message(STATUS "Build: Profile (gprof)")
  target_compile_options(${PROJECT_NAME} PRIVATE
    -O2 -g -pg -fno-omit-frame-pointer
  )
  target_link_options(${PROJECT_NAME} PRIVATE -pg)
else()
  message(STATUS "Build: default optimized build")
  target_compile_options(${PROJECT_NAME} PRIVATE -O2 -g)
endif()

if(BUILD_TESTING)
  # PIDFD-LIFECYCLE-EVAL-01: owned child, test-only lifecycle reference.
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(pidfd-lifecycle-reference tests/integration/test_pidfd_lifecycle_reference.c)
    target_compile_options(pidfd-lifecycle-reference PRIVATE -Wall -Wextra -Wpedantic -Werror -Wstrict-prototypes)
    add_test(NAME pidfd_lifecycle_reference COMMAND pidfd-lifecycle-reference)
    set_tests_properties(pidfd_lifecycle_reference PROPERTIES TIMEOUT 10 SKIP_RETURN_CODE 77)
  endif()

  # REFLINK-EVAL-01: private whole-file clone and copy-on-write reference.
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(reflink-reference tests/integration/test_reflink_reference.c)
    target_compile_options(reflink-reference PRIVATE -Wall -Wextra -Wpedantic -Werror -Wstrict-prototypes)
    add_test(NAME reflink_reference COMMAND reflink-reference)
    add_test(NAME reflink_fallback_reference COMMAND reflink-reference --fallback)
    add_test(NAME reflink_error_reference COMMAND reflink-reference --errors)
    set_tests_properties(reflink_reference reflink_fallback_reference reflink_error_reference PROPERTIES TIMEOUT 15 SKIP_RETURN_CODE 77)
  endif()

  # SPARSE-EXTENT-EVAL-01: private disposable extent evidence.
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(sparse-extent-reference tests/integration/test_sparse_extent_reference.c)
    target_compile_options(sparse-extent-reference PRIVATE -Wall -Wextra -Wpedantic -Werror -Wstrict-prototypes)
    add_test(NAME sparse_extent_reference COMMAND sparse-extent-reference)
    add_test(NAME sparse_extent_boundaries COMMAND sparse-extent-reference --boundaries)
    add_test(NAME sparse_extent_copy COMMAND sparse-extent-reference --copy)
    add_test(NAME sparse_extent_measurement COMMAND sparse-extent-reference --measure)
    add_test(NAME sparse_extent_rejections COMMAND sparse-extent-reference --reject)
    set_tests_properties(sparse_extent_rejections PROPERTIES TIMEOUT 15)
    set_tests_properties(sparse_extent_reference sparse_extent_boundaries sparse_extent_copy sparse_extent_measurement PROPERTIES TIMEOUT 15 SKIP_RETURN_CODE 77)
  endif()

  # WATCH-OVERFLOW-EVAL-01: private Linux queue only; no sysctl writes.
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(inotify-overflow-reference tests/integration/test_inotify_overflow_reference.c)
    target_compile_options(inotify-overflow-reference PRIVATE -Wall -Wextra -Wpedantic -Werror -Wstrict-prototypes)
    add_test(NAME inotify_overflow_reference COMMAND inotify-overflow-reference)
    add_test(NAME inotify_rescan_reference COMMAND inotify-overflow-reference --rescan)
    add_test(NAME inotify_rescan_invalidation_reference COMMAND inotify-overflow-reference --invalidate)
    add_test(NAME inotify_restart_reference COMMAND inotify-overflow-reference --restart)
    add_test(NAME inotify_restart_gap_reference COMMAND inotify-overflow-reference --gap)
    set_tests_properties(inotify_overflow_reference inotify_rescan_reference
      inotify_rescan_invalidation_reference inotify_restart_reference
      inotify_restart_gap_reference PROPERTIES TIMEOUT 30 SKIP_RETURN_CODE 77)
  endif()

  # STATX-MOUNT-EVAL-01: installed libc/UAPI, private fixture only.
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(CheckCSourceCompiles)
    check_c_source_compiles([=[
      #define _GNU_SOURCE
      #include <fcntl.h>
      #include <sys/stat.h>
      int main(void) {
        struct statx value = {0};
        return statx(AT_FDCWD, ".", AT_SYMLINK_NOFOLLOW,
          STATX_TYPE | STATX_INO | STATX_MNT_ID, &value) +
          (int)(value.stx_mnt_id + AT_EMPTY_PATH);
      }
    ]=] FC_HAVE_STATX_MOUNT_REFERENCE)
    add_executable(statx-mount-reference tests/integration/test_statx_mount_reference.c)
    if(FC_HAVE_STATX_MOUNT_REFERENCE)
      target_compile_definitions(statx-mount-reference PRIVATE FC_HAVE_STATX_MOUNT_REFERENCE)
    endif()
    target_compile_options(statx-mount-reference PRIVATE -Wall -Wextra -Wpedantic -Werror -Wstrict-prototypes)
    add_test(NAME statx_mount_reference COMMAND statx-mount-reference)
    add_test(NAME statx_cross_mount_reference COMMAND statx-mount-reference --cross-mount)
    add_test(NAME statx_error_reference COMMAND statx-mount-reference --errors)
    add_test(NAME statx_fdinfo_reference COMMAND statx-mount-reference --fdinfo)
    add_test(NAME statx_timing_reference COMMAND statx-mount-reference --timing)
    add_test(NAME statx_same_device_reference COMMAND statx-mount-reference --same-device)
    set_tests_properties(statx_mount_reference statx_cross_mount_reference
      statx_error_reference statx_fdinfo_reference statx_timing_reference
      statx_same_device_reference PROPERTIES TIMEOUT 10 SKIP_RETURN_CODE 77)
  endif()

  # LANDLOCK-PREVIEW-EVAL-01: test-only installed UAPI and child restriction.
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(CheckCSourceCompiles)
    check_c_source_compiles([=[
      #include <linux/landlock.h>
      #include <sys/syscall.h>
      int main(void) {
        struct landlock_ruleset_attr rules = {0};
        return (int)(rules.handled_access_fs + SYS_landlock_create_ruleset +
          SYS_landlock_restrict_self + LANDLOCK_CREATE_RULESET_VERSION +
          LANDLOCK_ACCESS_FS_WRITE_FILE + LANDLOCK_ACCESS_FS_REMOVE_FILE);
      }
    ]=] FC_HAVE_LANDLOCK_REFERENCE)
    add_executable(landlock-preview-reference tests/integration/test_landlock_preview_reference.c)
    if(FC_HAVE_LANDLOCK_REFERENCE)
      target_compile_definitions(landlock-preview-reference PRIVATE FC_HAVE_LANDLOCK_REFERENCE)
    endif()
    target_compile_options(landlock-preview-reference PRIVATE -Wall -Wextra -Wpedantic -Werror -Wstrict-prototypes)
    add_test(NAME landlock_preview_reference COMMAND landlock-preview-reference)
    add_test(NAME landlock_preview_extended COMMAND landlock-preview-reference --extended)
    add_test(NAME landlock_descriptor_reference COMMAND landlock-preview-reference --descriptor)
    add_test(NAME landlock_descendant_reference COMMAND landlock-preview-reference --descendant)
    add_test(NAME landlock_exec_reference COMMAND landlock-preview-reference --exec-reference)
    set_tests_properties(landlock_preview_reference landlock_preview_extended
      landlock_descriptor_reference landlock_descendant_reference landlock_exec_reference
      PROPERTIES TIMEOUT 10 SKIP_RETURN_CODE 77)
  endif()

  # UPSTREAM-COST-01: synthetic integer cost reference, never a product target.
  add_executable(upstream_tco_reference tests/reference/upstream_tco_reference.c)
  target_compile_options(upstream_tco_reference PRIVATE -Wall -Wextra -Wpedantic -Werror)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(upstream_tco_reference PRIVATE
      -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
    target_link_options(upstream_tco_reference PRIVATE -fsanitize=address,undefined)
  endif()
  add_test(NAME upstream_tco_reference COMMAND upstream_tco_reference)
  set_tests_properties(upstream_tco_reference PROPERTIES TIMEOUT 10)

  # UPSTREAM-MAINT-01: record observed environment, not version-based capability.
  add_test(NAME upstream_environment_report
    COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scripts/tests/report_upstream_environment.sh"
            "${CMAKE_COMMAND}" "${CMAKE_C_COMPILER}" "${CMAKE_LINKER}")
  set_tests_properties(upstream_environment_report PROPERTIES TIMEOUT 10)
  add_test(NAME upstream_environment_comparison
    COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scripts/tests/test_upstream_environment_comparison.sh"
            "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_COMMAND}"
            "${CMAKE_C_COMPILER}" "${CMAKE_LINKER}")
  set_tests_properties(upstream_environment_comparison PROPERTIES TIMEOUT 10)

  # STATIC-LIB-01: private reference archive of existing pure modules.
  # Never install, export, or link this target into the production CLI.
  add_library(fc_classifier_reference STATIC
    src/classifier/classifier.c src/utils/hashmap.c)
  target_include_directories(fc_classifier_reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(fc_classifier_reference PRIVATE -Wall -Wextra -Wpedantic -Werror)
  add_executable(static_classifier_consumer tests/reference/static_classifier_consumer.c)
  target_include_directories(static_classifier_consumer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(static_classifier_consumer PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_link_libraries(static_classifier_consumer PRIVATE fc_classifier_reference)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    foreach(target fc_classifier_reference static_classifier_consumer)
      target_compile_options(${target} PRIVATE
        -g -fsanitize=address,undefined -fno-sanitize-recover=all
        -fno-omit-frame-pointer)
    endforeach()
    target_link_options(static_classifier_consumer PRIVATE -fsanitize=address,undefined)
  endif()
  add_test(NAME static_classifier_consumer COMMAND static_classifier_consumer)
  set_tests_properties(static_classifier_consumer PROPERTIES TIMEOUT 10)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Repeat the existing binary dependency oracle in the standard CTest matrix.
    add_test(NAME static_classifier_dependency_boundary
      COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scripts/tests/check_static_classifier_dependencies.sh"
              $<TARGET_FILE:fc_classifier_reference>)
    set_tests_properties(static_classifier_dependency_boundary PROPERTIES TIMEOUT 10)
    add_test(NAME static_classifier_artifact_isolation
      COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scripts/tests/check_static_classifier_artifacts.sh"
              "${CMAKE_COMMAND}" "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_BINARY_DIR}"
              $<TARGET_FILE:${PROJECT_NAME}> $<TARGET_FILE:fc_classifier_reference>
              "${CMAKE_GENERATOR}" "${CMAKE_C_COMPILER}" "${FC_C_STANDARD}")
    set_tests_properties(static_classifier_artifact_isolation PROPERTIES
      TIMEOUT 60 RUN_SERIAL TRUE)
  endif()

  # SHARED-LIB-01: private PIC shared reference; never install or export.
  add_library(fc_classifier_shared_reference SHARED
    src/classifier/classifier.c src/utils/hashmap.c
    tests/reference/shared_classifier_reference.c)
  target_include_directories(fc_classifier_shared_reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  set_target_properties(fc_classifier_shared_reference PROPERTIES
    POSITION_INDEPENDENT_CODE ON C_VISIBILITY_PRESET hidden
    VERSION 0.1.0 SOVERSION 0)
  target_compile_options(fc_classifier_shared_reference PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_link_options(fc_classifier_shared_reference PRIVATE
    -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack)
  # Clang's Debug DSO resolves sanitizer hooks from its instrumented host.
  # Keep the undefined-symbol link gate for all other configurations.
  if(NOT (CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_C_COMPILER_ID MATCHES "Clang"))
    target_link_options(fc_classifier_shared_reference PRIVATE -Wl,--no-undefined)
  endif()
  add_executable(shared_classifier_consumer tests/reference/shared_classifier_consumer.c)
  target_compile_options(shared_classifier_consumer PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_link_libraries(shared_classifier_consumer PRIVATE fc_classifier_shared_reference)
  set_target_properties(shared_classifier_consumer PROPERTIES BUILD_RPATH_USE_ORIGIN TRUE)
  add_test(NAME shared_classifier_consumer COMMAND shared_classifier_consumer)
  set_tests_properties(shared_classifier_consumer PROPERTIES TIMEOUT 10)
  add_executable(shared_classifier_runtime tests/reference/shared_classifier_runtime.c)
  target_compile_options(shared_classifier_runtime PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_link_libraries(shared_classifier_runtime PRIVATE ${CMAKE_DL_LIBS})
  add_dependencies(shared_classifier_runtime fc_classifier_shared_reference)
  add_test(NAME shared_classifier_runtime
    COMMAND shared_classifier_runtime $<TARGET_FILE:fc_classifier_shared_reference>)
  set_tests_properties(shared_classifier_runtime PROPERTIES TIMEOUT 10)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    foreach(target fc_classifier_shared_reference shared_classifier_consumer shared_classifier_runtime)
      target_compile_options(${target} PRIVATE
        -g -fsanitize=address,undefined -fno-sanitize-recover=all
        -fno-omit-frame-pointer)
      target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endforeach()
    set_tests_properties(shared_classifier_consumer shared_classifier_runtime PROPERTIES
      ENVIRONMENT "ASAN_OPTIONS=halt_on_error=1:detect_leaks=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
  endif()
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_test(NAME shared_classifier_elf_boundary
      COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scripts/tests/check_shared_classifier_elf.sh"
              $<TARGET_FILE:fc_classifier_shared_reference>
              $<TARGET_FILE:shared_classifier_consumer> "${CMAKE_C_COMPILER}")
    set_tests_properties(shared_classifier_elf_boundary PROPERTIES TIMEOUT 15)
    add_test(NAME shared_classifier_artifact_isolation
      COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scripts/tests/check_shared_classifier_artifacts.sh"
              "${CMAKE_COMMAND}" "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_BINARY_DIR}"
              $<TARGET_FILE:${PROJECT_NAME}> $<TARGET_FILE:fc_classifier_shared_reference>
              $<TARGET_FILE:shared_classifier_runtime> "${CMAKE_GENERATOR}"
              "${CMAKE_C_COMPILER}" "${FC_C_STANDARD}")
    set_tests_properties(shared_classifier_artifact_isolation PROPERTIES
      TIMEOUT 60 RUN_SERIAL TRUE)
  endif()

  # PLAN-REPORT-01: opt-in aggregate JSON with disposable preservation checks.
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME plan_report_json
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/test_plan_report_json.py
            $<TARGET_FILE:${PROJECT_NAME}>
  )
  set_tests_properties(plan_report_json PROPERTIES TIMEOUT 25)
  # CLI-ERROR-01: unknown options and excess roots get safe diagnostics.
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME cli_errors
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/test_cli_errors.py
            $<TARGET_FILE:${PROJECT_NAME}>
  )
  set_tests_properties(cli_errors PROPERTIES TIMEOUT 15)
  # CLI-VALUES-01: missing/empty option operands must fail before filesystem access.
  add_test(NAME cli_required_values
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/test_cli_required_values.py
            $<TARGET_FILE:${PROJECT_NAME}>
  )
  set_tests_properties(cli_required_values PROPERTIES TIMEOUT 15)
  # CLI-HELP-01: standalone help is successful and side-effect-free.
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME cli_help
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/test_cli_help.py
            $<TARGET_FILE:${PROJECT_NAME}>
  )
  set_tests_properties(cli_help PROPERTIES TIMEOUT 15)

  # CLI-ROOT-01: ambiguous root arguments must fail before config/filesystem access.
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME cli_root_duplicates
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/test_cli_root_duplicates.py
            $<TARGET_FILE:${PROJECT_NAME}>
  )
  set_tests_properties(cli_root_duplicates PROPERTIES TIMEOUT 15)

  # CLI-CONFIG-01: repeated config selectors fail before filesystem access.
  add_test(NAME cli_config_duplicates
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/test_cli_config_duplicates.py
            $<TARGET_FILE:${PROJECT_NAME}>
  )
  set_tests_properties(cli_config_duplicates PROPERTIES TIMEOUT 15)

  add_executable(test_hashmap tests/unit/test_hashmap.c src/utils/hashmap.c)
  target_include_directories(test_hashmap PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(test_hashmap PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -UNDEBUG)
  add_test(NAME hashmap_unit COMMAND test_hashmap)

  add_executable(hashmap-benchmark tests/performance/hashmap_benchmark.c src/utils/hashmap.c)
  target_include_directories(hashmap-benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(hashmap-benchmark PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(hashmap-benchmark PRIVATE -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2)
    target_link_options(hashmap-benchmark PRIVATE -Wl,-z,relro,-z,now)
  endif()
  add_test(NAME hashmap_benchmark_smoke COMMAND hashmap-benchmark)
  set_tests_properties(hashmap_benchmark_smoke PROPERTIES TIMEOUT 15)

  add_executable(hashmap-capacity-benchmark tests/performance/hashmap_capacity_benchmark.c src/utils/hashmap.c)
  target_include_directories(hashmap-capacity-benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(hashmap-capacity-benchmark PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(hashmap-capacity-benchmark PRIVATE -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2)
    target_link_options(hashmap-capacity-benchmark PRIVATE -Wl,-z,relro,-z,now)
  endif()
  add_test(NAME hashmap_capacity_benchmark_smoke COMMAND hashmap-capacity-benchmark)
  set_tests_properties(hashmap_capacity_benchmark_smoke PROPERTIES TIMEOUT 15)

  add_executable(hashmap-classifier-benchmark tests/performance/hashmap_classifier_benchmark.c src/classifier/classifier.c src/utils/hashmap.c)
  target_include_directories(hashmap-classifier-benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(hashmap-classifier-benchmark PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(hashmap-classifier-benchmark PRIVATE -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2)
    target_link_options(hashmap-classifier-benchmark PRIVATE -Wl,-z,relro,-z,now)
  endif()
  add_test(NAME hashmap_classifier_benchmark_smoke COMMAND hashmap-classifier-benchmark)
  set_tests_properties(hashmap_classifier_benchmark_smoke PROPERTIES TIMEOUT 15)

  add_executable(hashmap-collision-benchmark tests/performance/hashmap_collision_benchmark.c src/utils/hashmap.c)
  target_include_directories(hashmap-collision-benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(hashmap-collision-benchmark PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(hashmap-collision-benchmark PRIVATE -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2)
    target_link_options(hashmap-collision-benchmark PRIVATE -Wl,-z,relro,-z,now)
  endif()
  add_test(NAME hashmap_collision_benchmark_smoke COMMAND hashmap-collision-benchmark)
  set_tests_properties(hashmap_collision_benchmark_smoke PROPERTIES TIMEOUT 15)

  add_executable(path-allocation-benchmark tests/performance/path_allocation_benchmark.c)
  target_include_directories(path-allocation-benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(path-allocation-benchmark PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(path-allocation-benchmark PRIVATE -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2)
    target_link_options(path-allocation-benchmark PRIVATE -Wl,-z,relro,-z,now)
  endif()
  add_test(NAME path_allocation_benchmark_smoke COMMAND path-allocation-benchmark)
  set_tests_properties(path_allocation_benchmark_smoke PROPERTIES TIMEOUT 15)

  add_executable(path-arena-benchmark tests/performance/path_arena_benchmark.c)
  target_include_directories(path-arena-benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(path-arena-benchmark PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(path-arena-benchmark PRIVATE -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2)
    target_link_options(path-arena-benchmark PRIVATE -Wl,-z,relro,-z,now)
  endif()
  add_test(NAME path_arena_benchmark_smoke COMMAND path-arena-benchmark)
  set_tests_properties(path_arena_benchmark_smoke PROPERTIES TIMEOUT 15)

  add_executable(string-extension-benchmark tests/performance/string_extension_benchmark.c src/classifier/classifier.c src/utils/hashmap.c)
  target_include_directories(string-extension-benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(string-extension-benchmark PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(string-extension-benchmark PRIVATE -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2)
    target_link_options(string-extension-benchmark PRIVATE -Wl,-z,relro,-z,now)
  endif()
  add_test(NAME string_extension_benchmark_smoke COMMAND string-extension-benchmark)
  set_tests_properties(string_extension_benchmark_smoke PROPERTIES TIMEOUT 15)

  add_executable(scanner-reference-benchmark tests/performance/scanner_reference_benchmark.c src/scanner/scanner.c)
  target_include_directories(scanner-reference-benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(scanner-reference-benchmark PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(scanner-reference-benchmark PRIVATE -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2)
    target_link_options(scanner-reference-benchmark PRIVATE -Wl,-z,relro,-z,now)
  endif()
  add_test(NAME scanner_reference_benchmark_smoke COMMAND scanner-reference-benchmark)
  set_tests_properties(scanner_reference_benchmark_smoke PROPERTIES TIMEOUT 15)
  add_executable(cpu-opt-reference tests/performance/cpu_opt_reference.c tests/performance/cpu_opt_kernels.c)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$" AND CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang)$")
    target_compile_options(cpu-opt-reference PRIVATE -march=x86-64 -mtune=generic -mno-avx -mno-avx2 -mno-popcnt)
  endif()
  if(CMAKE_BUILD_TYPE STREQUAL "Release" AND CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(cpu-opt-reference PRIVATE -fopt-info-vec-optimized)
  endif()
  target_compile_options(cpu-opt-reference PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(cpu-opt-reference PRIVATE
      -O0 -g3 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
    target_link_options(cpu-opt-reference PRIVATE -fsanitize=address,undefined)
  endif()
  add_test(NAME cpu_opt_reference COMMAND cpu-opt-reference)
  set_tests_properties(cpu_opt_reference PROPERTIES TIMEOUT 15)
  add_executable(cache-layout-reference tests/performance/cache_layout_reference.c)
  target_link_libraries(cache-layout-reference PRIVATE Threads::Threads)
  target_include_directories(cache-layout-reference PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(cache-layout-reference PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
  # Instrument this experimental target itself in Debug, not just the CLI.
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(cache-layout-reference PRIVATE
      -O0 -g3 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
    target_link_options(cache-layout-reference PRIVATE -fsanitize=address,undefined)
  endif()
  add_test(NAME cache_layout_reference COMMAND cache-layout-reference)
  set_tests_properties(cache_layout_reference PROPERTIES TIMEOUT 15)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(batch-io-reference tests/performance/batch_io_reference.c)
    target_compile_options(batch-io-reference PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
    add_test(NAME batch_io_reference COMMAND batch-io-reference)
    set_tests_properties(batch_io_reference PROPERTIES TIMEOUT 15)
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(CheckCSourceCompiles)
    check_c_source_compiles([=[
      #define _GNU_SOURCE
      #include <linux/io_uring.h>
      #include <sys/syscall.h>
      int main(void) {
        struct io_uring_params params = {0};
        struct io_uring_probe probe = {0};
        return (int)(params.sq_entries + probe.ops_len +
          SYS_io_uring_setup + SYS_io_uring_register + SYS_io_uring_enter +
          IORING_FEAT_SINGLE_MMAP + IORING_ENTER_GETEVENTS + IORING_REGISTER_PROBE +
          IORING_OP_NOP + IORING_OP_READV + IORING_OP_WRITEV + IORING_OP_FSYNC +
          IO_URING_OP_SUPPORTED);
      }
    ]=] FC_HAVE_URING_PROBE)
    add_executable(linux-uring-capability tests/performance/linux_uring_capability.c)
    if(FC_HAVE_URING_PROBE)
      target_compile_definitions(linux-uring-capability PRIVATE FC_HAVE_URING_PROBE)
    endif()
    target_compile_options(linux-uring-capability PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
    add_test(NAME linux_uring_capability COMMAND linux-uring-capability)
    set_tests_properties(linux_uring_capability PROPERTIES TIMEOUT 10 SKIP_RETURN_CODE 77)
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(CheckSymbolExists)
    set(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
    check_symbol_exists(copy_file_range "unistd.h" FC_HAVE_COPY_FILE_RANGE)
    unset(CMAKE_REQUIRED_DEFINITIONS)
    if(FC_HAVE_COPY_FILE_RANGE)
      add_executable(linux-io-reference tests/performance/linux_io_reference.c)
      target_compile_options(linux-io-reference PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
      add_test(NAME linux_io_reference COMMAND linux-io-reference)
      set_tests_properties(linux_io_reference PROPERTIES TIMEOUT 15 SKIP_RETURN_CODE 77)
    endif()
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(CheckIncludeFile)
    check_include_file("linux/openat2.h" FC_HAVE_OPENAT2_HEADER)
    if(FC_HAVE_OPENAT2_HEADER)
      add_executable(linux-path-reference tests/integration/test_linux_path_reference.c)
      target_compile_options(linux-path-reference PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
      add_test(NAME linux_path_reference COMMAND linux-path-reference)
      set_tests_properties(linux_path_reference PROPERTIES TIMEOUT 15 SKIP_RETURN_CODE 77)
    endif()
  endif()

  add_executable(test_classifier tests/unit/test_classifier.c src/classifier/classifier.c src/utils/hashmap.c)
  target_include_directories(test_classifier PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(test_classifier PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -UNDEBUG)
  add_test(NAME classifier_unit COMMAND test_classifier)

  add_executable(test_config_destination
    tests/integration/test_config_destination.c
    src/config/config.c
    src/utils/hashmap.c
  )
  target_include_directories(test_config_destination PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(test_config_destination PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -UNDEBUG)
  add_test(NAME config_destination_security COMMAND test_config_destination)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang)$")
    add_executable(test_config_stream_fault tests/integration/test_config_stream_fault.c src/config/config.c src/utils/hashmap.c)
    target_include_directories(test_config_stream_fault PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_compile_options(test_config_stream_fault PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
    target_link_options(test_config_stream_fault PRIVATE -Wl,--wrap=fdopen -Wl,--wrap=malloc -Wl,--wrap=free -Wl,--wrap=open)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      target_compile_options(test_config_stream_fault PRIVATE -O0 -g3 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
      target_link_options(test_config_stream_fault PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME config_stream_fault COMMAND test_config_stream_fault)
    set_tests_properties(config_stream_fault PROPERTIES TIMEOUT 15)
  endif()

  add_executable(test_queue tests/unit/test_queue.c src/core/lf_queue.c)
  target_include_directories(test_queue PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(test_queue PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -UNDEBUG)
  target_link_libraries(test_queue PRIVATE Threads::Threads)
  add_test(NAME queue_unit COMMAND test_queue)
  set_tests_properties(queue_unit PROPERTIES TIMEOUT 30)

  # Independent in-memory queue baseline; no filesystem scale benchmark.
  add_executable(queue-benchmark tests/performance/queue_benchmark.c src/core/lf_queue.c)
  target_include_directories(queue-benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(queue-benchmark PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(queue-benchmark PRIVATE -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2)
    target_link_options(queue-benchmark PRIVATE -Wl,-z,relro,-z,now)
  endif()
  add_test(NAME queue_benchmark_smoke COMMAND queue-benchmark)
  set_tests_properties(queue_benchmark_smoke PROPERTIES TIMEOUT 15)

  add_executable(queue-concurrent-benchmark tests/performance/queue_concurrent_benchmark.c src/core/lf_queue.c)
  target_include_directories(queue-concurrent-benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(queue-concurrent-benchmark PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
  target_link_libraries(queue-concurrent-benchmark PRIVATE Threads::Threads)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(queue-concurrent-benchmark PRIVATE -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2)
    target_link_options(queue-concurrent-benchmark PRIVATE -Wl,-z,relro,-z,now)
  endif()
  add_test(NAME queue_concurrent_benchmark_smoke COMMAND queue-concurrent-benchmark)
  set_tests_properties(queue_concurrent_benchmark_smoke PROPERTIES TIMEOUT 25)

  # Test-only candidate: compile the same source and harness against a
  # generated private header. Production includes and ABI remain untouched.
  set(queue_layout_header "${CMAKE_CURRENT_SOURCE_DIR}/include/core/lf_queue.h")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${queue_layout_header}")
  file(READ "${queue_layout_header}" queue_layout_original)
  string(REPLACE "    atomic_int head;\n    atomic_int tail;"
    "    _Alignas(64) atomic_int head;\n    _Alignas(64) atomic_int tail;"
    queue_layout_candidate "${queue_layout_original}")
  if(queue_layout_candidate STREQUAL queue_layout_original)
    message(FATAL_ERROR "Queue cache-layout experiment requires review: header anchor changed")
  endif()
  set(queue_layout_dir "${CMAKE_CURRENT_BINARY_DIR}/queue-layout-candidate")
  file(MAKE_DIRECTORY "${queue_layout_dir}/core")
  file(WRITE "${queue_layout_dir}/core/lf_queue.h" "${queue_layout_candidate}")
  add_executable(queue-padded-benchmark tests/performance/queue_concurrent_benchmark.c src/core/lf_queue.c)
  target_include_directories(queue-padded-benchmark PRIVATE "${queue_layout_dir}" "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_definitions(queue-padded-benchmark PRIVATE FC_QUEUE_PADDED_EXPERIMENT)
  target_compile_options(queue-padded-benchmark PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
  target_link_libraries(queue-padded-benchmark PRIVATE Threads::Threads)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(queue-padded-benchmark PRIVATE -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2)
    target_link_options(queue-padded-benchmark PRIVATE -Wl,-z,relro,-z,now)
  endif()
  add_test(NAME queue_padded_benchmark_smoke COMMAND queue-padded-benchmark)
  set_tests_properties(queue_padded_benchmark_smoke PROPERTIES TIMEOUT 25)

  # Invalid affinity input must fail before any transfer or affinity change.
  foreach(queue_target queue-concurrent-benchmark queue-padded-benchmark)
    add_test(NAME ${queue_target}_bad_cpu COMMAND ${queue_target} -1 0)
    add_test(NAME ${queue_target}_duplicate_cpu COMMAND ${queue_target} 0 0)
    add_test(NAME ${queue_target}_missing_cpu COMMAND ${queue_target} 0)
    add_test(NAME ${queue_target}_overflow_cpu COMMAND ${queue_target} 9999999999999999999999 0)
    set_tests_properties(${queue_target}_bad_cpu ${queue_target}_duplicate_cpu
      ${queue_target}_missing_cpu ${queue_target}_overflow_cpu
      PROPERTIES WILL_FAIL TRUE TIMEOUT 5)
  endforeach()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(test_linux_scanner_cloexec tests/integration/test_linux_scanner_cloexec.c src/scanner/scanner.c)
    target_include_directories(test_linux_scanner_cloexec PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_compile_options(test_linux_scanner_cloexec PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      target_compile_options(test_linux_scanner_cloexec PRIVATE -O0 -g3 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
      target_link_options(test_linux_scanner_cloexec PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME linux_scanner_cloexec COMMAND test_linux_scanner_cloexec)
    set_tests_properties(linux_scanner_cloexec PROPERTIES TIMEOUT 15)
  endif()
  add_executable(test_scanner tests/integration/test_scanner.c src/scanner/scanner.c)
  target_include_directories(test_scanner PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_definitions(test_scanner PRIVATE FC_SCANNER_TESTING)
  target_compile_options(test_scanner PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -UNDEBUG)
  add_test(NAME scanner_security COMMAND test_scanner)
  set_tests_properties(scanner_security PROPERTIES TIMEOUT 30)

  add_executable(test_mover
    tests/integration/test_mover.c
    src/mover/mover.c
    src/utils/fs_utils.c
    src/journal/journal.c
  )
  target_include_directories(test_mover PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_link_libraries(test_mover PRIVATE Threads::Threads)
  target_compile_definitions(test_mover PRIVATE FC_TESTING)
  target_compile_options(test_mover PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -UNDEBUG)
  add_test(NAME mover_security COMMAND test_mover)

  # FS-01B: intentional known-risk characterization in private fixtures only.
  # PASS means a post-identity-check entry substitution is reproducible,
  # NOT that production source retirement is safe. See docs/security/FS_01...
  add_executable(test_fs01b_retirement_race
    tests/integration/test_fs01b_retirement_race.c
    src/mover/mover.c
    src/journal/journal.c
  )
  target_include_directories(test_fs01b_retirement_race PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  target_link_libraries(test_fs01b_retirement_race PRIVATE Threads::Threads)
  target_compile_definitions(test_fs01b_retirement_race PRIVATE FC_TESTING)
  target_compile_options(test_fs01b_retirement_race PRIVATE
    -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -UNDEBUG
  )
  add_test(NAME fs01b_known_race_characterization COMMAND test_fs01b_retirement_race)

  # FS-01B-GUARD tests the production mover (NO FC_TESTING):
  # prevent source retirement in group/other-writable directories.
  add_executable(test_fs01b_source_permissions
    tests/integration/test_fs01b_source_permissions.c
    src/mover/mover.c
    src/journal/journal.c
  )
  target_include_directories(test_fs01b_source_permissions PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  target_link_libraries(test_fs01b_source_permissions PRIVATE Threads::Threads)
  target_compile_options(test_fs01b_source_permissions PRIVATE
    -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -UNDEBUG
  )
  add_test(NAME fs01b_untrusted_source_guard COMMAND test_fs01b_source_permissions)

  add_executable(test_journal
    tests/integration/test_journal.c
    src/journal/journal.c
  )
  target_include_directories(test_journal PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_link_libraries(test_journal PRIVATE Threads::Threads)
  target_compile_definitions(test_journal PRIVATE FC_TESTING)
  target_compile_options(test_journal PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -UNDEBUG)
  add_test(NAME journal_recovery_security COMMAND test_journal)

  # FCJ3 parser remains isolated from production journal recovery until the
  # writer/staging/recovery phases are separately reviewed and enabled.
  add_executable(test_fcj3_parser
    tests/unit/test_fcj3_parser.c
    src/journal/fcj3_parser.c
  )
  target_include_directories(test_fcj3_parser PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  target_compile_options(test_fcj3_parser PRIVATE
    -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes -UNDEBUG
  )
  add_test(NAME fcj3_parser_unit COMMAND test_fcj3_parser)

  # FCJ4 pure whole-buffer replay remains isolated from production recovery.
  add_executable(test_fcj4_parser
    tests/unit/test_fcj4_parser.c
    src/journal/fcj4_parser.c
  )
  target_include_directories(test_fcj4_parser PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  target_compile_options(test_fcj4_parser PRIVATE
    -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes -UNDEBUG
  )
  add_test(NAME fcj4_parser_unit COMMAND test_fcj4_parser)

  # RUST-01A: C-owned, test-only parser shim. NOT a product ABI.
  # Rust extern-C integration is a separately reviewed future milestone.
  add_executable(test_fcj4_parser_shim
    tests/unit/test_fcj4_parser_shim.c
    tests/support/fcj4_parser_test_shim.c
    src/journal/fcj4_parser.c
  )
  target_include_directories(test_fcj4_parser_shim PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/support"
  )
  target_compile_options(test_fcj4_parser_shim PRIVATE
    -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes -UNDEBUG
  )
  add_test(NAME fcj4_parser_test_shim_unit COMMAND test_fcj4_parser_shim)

  # FCJ4 canonical serializer and in-memory state remain isolated from
  # journal-file I/O and production recovery.
  add_executable(test_fcj4_writer
    tests/unit/test_fcj4_writer.c
    src/journal/fcj4_writer.c
    src/journal/fcj4_parser.c
  )
  target_include_directories(test_fcj4_writer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  target_compile_options(test_fcj4_writer PRIVATE
    -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes -UNDEBUG
  )
  add_test(NAME fcj4_writer_unit COMMAND test_fcj4_writer)

  # Linux-only FCJ4 append adapter for disposable test journals. This target
  # must never be linked into the production executable or expose a CLI.
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(test_fcj4_test_append
      tests/integration/test_fcj4_test_append.c
      tests/integration/test_fcj4_test_append_concurrency.c
      tests/support/fcj4_test_append.c
      src/journal/fcj4_writer.c
      src/journal/fcj4_parser.c
    )
    target_include_directories(test_fcj4_test_append PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/support"
    )
    # pthreads are required only by this Linux-only test executable.
    target_link_libraries(test_fcj4_test_append PRIVATE Threads::Threads)
    target_compile_options(test_fcj4_test_append PRIVATE
      -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes -UNDEBUG
    )
    add_test(NAME fcj4_test_append_integration COMMAND test_fcj4_test_append)
    set_tests_properties(fcj4_test_append_integration PROPERTIES TIMEOUT 30)

    # Restart/replay corruption matrix uses only serializer/parser APIs and
    # private disposable files; it is test-only and never linked into production.
    add_executable(test_fcj4_restart_matrix
      tests/integration/test_fcj4_restart_matrix.c
      src/journal/fcj4_writer.c
      src/journal/fcj4_parser.c
    )
    target_include_directories(test_fcj4_restart_matrix PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_compile_options(test_fcj4_restart_matrix PRIVATE
      -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes -UNDEBUG
    )
    add_test(NAME fcj4_restart_matrix COMMAND test_fcj4_restart_matrix)
    set_tests_properties(fcj4_restart_matrix PROPERTIES TIMEOUT 30)

    # Focused concurrency-only entry point keeps TSan away from the expensive
    # maximum-record and exhaustive byte-boundary cases in the full harness.
    add_executable(test_fcj4_test_append_concurrency
      tests/integration/test_fcj4_test_append_concurrency_main.c
      tests/integration/test_fcj4_test_append_concurrency.c
      tests/support/fcj4_test_append.c
      src/journal/fcj4_writer.c
      src/journal/fcj4_parser.c
    )
    target_include_directories(test_fcj4_test_append_concurrency PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/support"
    )
    target_link_libraries(test_fcj4_test_append_concurrency PRIVATE Threads::Threads)
    target_compile_options(test_fcj4_test_append_concurrency PRIVATE
      -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes -UNDEBUG
    )
    add_test(NAME fcj4_test_append_concurrency
      COMMAND test_fcj4_test_append_concurrency
    )
    set_tests_properties(fcj4_test_append_concurrency PROPERTIES TIMEOUT 30)
  endif()

  add_executable(test_fcj3_sha256
    tests/unit/test_fcj3_sha256.c
    src/journal/fcj3_sha256.c
  )
  target_include_directories(test_fcj3_sha256 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  target_compile_options(test_fcj3_sha256 PRIVATE
    -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes -UNDEBUG
  )
  add_test(NAME fcj3_sha256_unit COMMAND test_fcj3_sha256)

  # Pure FCJ3 recovery classification is read-only and isolated from all
  # filesystem, journal-writer, staging, and production mover modules.
  add_executable(test_fcj3_recovery_classifier
    tests/unit/test_fcj3_recovery_classifier.c
    src/journal/fcj3_recovery_classifier.c
  )
  target_include_directories(test_fcj3_recovery_classifier PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  target_compile_options(test_fcj3_recovery_classifier PRIVATE
    -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes -UNDEBUG
  )
  add_test(NAME fcj3_recovery_classifier_unit
    COMMAND test_fcj3_recovery_classifier
  )

  # Operator-invoked FCJ3 diagnostic report. This executable is deliberately
  # separate from file-cleaner and is never installed or used by production
  # FCJ2/legacy recovery.
  add_executable(fcj3_recovery_report
    src/tools/fcj3_recovery_report.c
    src/journal/fcj3_parser.c
    src/journal/fcj3_recovery_classifier.c
    src/journal/fcj3_recovery_evidence.c
  )
  set_target_properties(fcj3_recovery_report PROPERTIES OUTPUT_NAME fcj3-recovery-report)
  target_include_directories(fcj3_recovery_report PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  target_compile_options(fcj3_recovery_report PRIVATE
    -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes
  )

  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME fcj3_recovery_report_integration
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/test_fcj3_recovery_report.py
            $<TARGET_FILE:fcj3_recovery_report>
  )
  set_tests_properties(fcj3_recovery_report_integration PROPERTIES TIMEOUT 15)

  # Read-only FCJ3 filesystem evidence collection is isolated from production.
  add_executable(test_fcj3_recovery_evidence
    tests/integration/test_fcj3_recovery_evidence.c
    src/journal/fcj3_recovery_evidence.c
  )
  target_include_directories(test_fcj3_recovery_evidence PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  target_compile_options(test_fcj3_recovery_evidence PRIVATE
    -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes -UNDEBUG
  )
  add_test(NAME fcj3_recovery_evidence_integration
    COMMAND test_fcj3_recovery_evidence
  )
  set_tests_properties(fcj3_recovery_evidence_integration PROPERTIES TIMEOUT 15)

  # FCJ3 parser, operation context, append adapter, and staging lifecycle
  # remain test-only. Production use waits for the recovery phase.
  add_executable(test_fcj3_writer
    tests/unit/test_fcj3_writer.c
    src/journal/fcj3_writer.c
    src/journal/fcj3_parser.c
  )
  target_include_directories(test_fcj3_writer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  target_compile_definitions(test_fcj3_writer PRIVATE FC_TESTING)
  target_compile_options(test_fcj3_writer PRIVATE
    -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes -UNDEBUG
  )
  add_test(NAME fcj3_writer_unit COMMAND test_fcj3_writer)
  set_tests_properties(fcj3_writer_unit PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 10)

  # Durable FCJ3 append adapter is isolated to disposable integration fixtures.
  # It must never be linked into the production executable before recovery exists.
  add_executable(test_fcj3_journal
    tests/integration/test_fcj3_journal.c
    src/journal/fcj3_journal.c
    src/journal/fcj3_writer.c
    src/journal/fcj3_parser.c
  )
  target_include_directories(test_fcj3_journal PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  target_link_libraries(test_fcj3_journal PRIVATE Threads::Threads)
  target_compile_definitions(test_fcj3_journal PRIVATE FC_TESTING)
  target_compile_options(test_fcj3_journal PRIVATE
    -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes -UNDEBUG
  )
  add_test(NAME fcj3_journal_integration COMMAND test_fcj3_journal)
  set_tests_properties(fcj3_journal_integration PROPERTIES TIMEOUT 20)

  # Full FCJ3 staging lifecycle remains isolated from production sources.
  add_executable(test_fcj3_staging
    tests/integration/test_fcj3_staging.c
    src/journal/fcj3_stage.c
    src/journal/fcj3_sha256.c
    src/journal/fcj3_journal.c
    src/journal/fcj3_writer.c
    src/journal/fcj3_parser.c
  )
  target_include_directories(test_fcj3_staging PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  target_link_libraries(test_fcj3_staging PRIVATE Threads::Threads)
  target_compile_definitions(test_fcj3_staging PRIVATE FC_TESTING)
  target_compile_options(test_fcj3_staging PRIVATE
    -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes -UNDEBUG
  )
  add_test(NAME fcj3_staging_integration COMMAND test_fcj3_staging)
  set_tests_properties(fcj3_staging_integration PROPERTIES TIMEOUT 20)

  # FCJ3 process termination tests use disposable journals and never invoke
  # production recovery or mutate the production mover/journal path.
  add_executable(test_fcj3_process_crash
    tests/integration/test_fcj3_process_crash.c
    src/journal/fcj3_stage.c
    src/journal/fcj3_sha256.c
    src/journal/fcj3_journal.c
    src/journal/fcj3_writer.c
    src/journal/fcj3_parser.c
  )
  target_include_directories(test_fcj3_process_crash PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  target_link_libraries(test_fcj3_process_crash PRIVATE Threads::Threads)
  target_compile_definitions(test_fcj3_process_crash PRIVATE FC_TESTING)
  target_compile_options(test_fcj3_process_crash PRIVATE
    -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes -UNDEBUG
  )
  add_test(NAME fcj3_process_crash COMMAND test_fcj3_process_crash)
  set_tests_properties(fcj3_process_crash PROPERTIES TIMEOUT 50)

  add_executable(test_process_crash_recovery
    tests/integration/test_process_crash_recovery.c
    src/mover/mover.c
    src/utils/fs_utils.c
    src/journal/journal.c
  )
  target_include_directories(test_process_crash_recovery PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  target_link_libraries(test_process_crash_recovery PRIVATE Threads::Threads)
  target_compile_definitions(test_process_crash_recovery PRIVATE FC_TESTING)
  target_compile_options(test_process_crash_recovery PRIVATE
    -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -UNDEBUG
  )
  add_test(NAME process_crash_recovery COMMAND test_process_crash_recovery)
  set_tests_properties(process_crash_recovery PROPERTIES TIMEOUT 40)

  # Runtime capability probe for the anonymous staging design. Unsupported
  # filesystems are reported as skipped; production code does not depend on it.
  add_executable(test_otmpfile_capability
    tests/integration/test_otmpfile_capability.c
  )
  target_compile_options(test_otmpfile_capability PRIVATE
    -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes
  )
  add_test(NAME otmpfile_capability COMMAND test_otmpfile_capability)
  set_tests_properties(otmpfile_capability PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 10)

  # Pair the running environment with actual disposable capability probes.
  # Missing compile-time optional targets are not successful runtime probes.
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(maintenance_io "-")
    set(maintenance_path "-")
    if(TARGET linux-io-reference)
      set(maintenance_io "$<TARGET_FILE:linux-io-reference>")
    endif()
    if(TARGET linux-path-reference)
      set(maintenance_path "$<TARGET_FILE:linux-path-reference>")
    endif()
    add_test(NAME upstream_capability_correlation
      COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scripts/tests/report_upstream_capabilities.sh"
              "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_COMMAND}" "${CMAKE_C_COMPILER}"
              "${CMAKE_LINKER}" $<TARGET_FILE:linux-uring-capability>
              "${maintenance_io}" "${maintenance_path}" $<TARGET_FILE:test_fcj3_writer>
              $<TARGET_FILE:test_otmpfile_capability>)
    set_tests_properties(upstream_capability_correlation PROPERTIES TIMEOUT 75 RUN_SERIAL TRUE)
    add_test(NAME upstream_capability_controls
      COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scripts/tests/test_upstream_capability_controls.sh"
              "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_COMMAND}" "${CMAKE_C_COMPILER}"
              "${CMAKE_LINKER}")
    set_tests_properties(upstream_capability_controls PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE)
  endif()

  add_executable(test_worker_lifecycle
    tests/integration/test_worker_lifecycle.c
    src/core/thread_pool.c
    src/core/lf_queue.c
    src/classifier/classifier.c
    src/utils/hashmap.c
    src/mover/mover.c
    src/utils/fs_utils.c
    src/journal/journal.c
  )
  target_include_directories(test_worker_lifecycle PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_link_libraries(test_worker_lifecycle PRIVATE Threads::Threads)
  target_compile_definitions(test_worker_lifecycle PRIVATE FC_TESTING)
  target_compile_options(test_worker_lifecycle PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -UNDEBUG)
  add_test(NAME worker_lifecycle COMMAND test_worker_lifecycle)
  set_tests_properties(worker_lifecycle PROPERTIES TIMEOUT 30)

  add_executable(test_watcher
    tests/integration/test_watcher.c
    src/watcher/watcher.c
    src/core/thread_pool.c
    src/core/lf_queue.c
    src/classifier/classifier.c
    src/utils/hashmap.c
    src/mover/mover.c
    src/utils/fs_utils.c
    src/journal/journal.c
  )
  target_include_directories(test_watcher PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_link_libraries(test_watcher PRIVATE Threads::Threads)
  target_compile_definitions(test_watcher PRIVATE FC_WATCHER_TESTING)
  target_compile_options(test_watcher PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -UNDEBUG)
  add_test(NAME watcher_integration COMMAND test_watcher)
  set_tests_properties(watcher_integration PROPERTIES TIMEOUT 30)

  # CLI-INPUT-01: malformed worker counts must fail before config/filesystem access.
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME cli_workers_invalid
    COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/system/test_cli_workers.py"
            "$<TARGET_FILE:${PROJECT_NAME}>"
  )
  set_tests_properties(cli_workers_invalid PROPERTIES TIMEOUT 15)

  # CLI-MODE-01: contradictory modes must fail before configuration or I/O.
  add_test(NAME cli_mode_conflicts
    COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/system/test_cli_mode_conflicts.py"
            "$<TARGET_FILE:${PROJECT_NAME}>"
  )
  set_tests_properties(cli_mode_conflicts PROPERTIES TIMEOUT 15)

  add_test(NAME cli_end_to_end
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/system/test_full_run.sh"
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${CMAKE_CURRENT_SOURCE_DIR}/configs/default.conf"
  )
  set_tests_properties(cli_end_to_end PROPERTIES TIMEOUT 30)

  find_package(Python3 COMPONENTS Interpreter REQUIRED)

  add_executable(create-files create_files.c)
  target_link_libraries(create-files PRIVATE Threads::Threads)
  target_compile_options(create-files PRIVATE
    -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes
  )
  add_test(NAME fixture_generator_safety
    COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/performance/test_create_files.py"
            "$<TARGET_FILE:create-files>"
  )
  set_tests_properties(fixture_generator_safety PROPERTIES TIMEOUT 60)
  add_test(NAME cli_benchmark_smoke
    COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/performance/benchmark_cli.py"
            --binary "$<TARGET_FILE:${PROJECT_NAME}>"
            --config "${CMAKE_CURRENT_SOURCE_DIR}/configs/default.conf"
            --files 30
            --runs 3
            --workers 2
            --diagnostics
            --output "${CMAKE_CURRENT_BINARY_DIR}/benchmark-smoke/cli"
  )
  set_tests_properties(cli_benchmark_smoke PROPERTIES TIMEOUT 60)

  # BENCH-SCALE-01: validate the bounded disposable Python enumeration
  # baseline in the existing CTest/CI matrix. This does not run scale
  # measurements or exercise the production C scanner.
  add_test(NAME bench_scale_baseline_unit
    COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/performance/test_scan_scale_baseline.py"
  )
  set_tests_properties(bench_scale_baseline_unit PROPERTIES TIMEOUT 30)

  # BENCH-SCALE-01: real C scanner, no production test hooks or worker/mover.
  add_executable(scanner-benchmark
    tests/performance/scanner_benchmark.c src/scanner/scanner.c)
  target_include_directories(scanner-benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(scanner-benchmark PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes)
  # Match the production target's portable Release hardening for this measurement.
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(scanner-benchmark PRIVATE -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2)
    target_link_options(scanner-benchmark PRIVATE -Wl,-z,relro,-z,now)
  endif()
  add_test(NAME native_scanner_benchmark_smoke
    COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/performance/benchmark_scanner.py"
            --binary "$<TARGET_FILE:scanner-benchmark>" --files 30 --runs 2)
  set_tests_properties(native_scanner_benchmark_smoke PROPERTIES TIMEOUT 30)
  add_test(NAME native_scanner_report_unit
    COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/performance/test_native_scanner_report.py")
  set_tests_properties(native_scanner_report_unit PROPERTIES TIMEOUT 15)

  add_test(NAME cli_move_benchmark_smoke
    COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/performance/benchmark_moves.py"
            --binary "$<TARGET_FILE:${PROJECT_NAME}>"
            --config "${CMAKE_CURRENT_SOURCE_DIR}/configs/default.conf"
            --files 10
            --bytes-per-file 128
            --runs 2
            --workers 2
            --diagnostics
            --output "${CMAKE_CURRENT_BINARY_DIR}/benchmark-smoke/moves"
  )
  set_tests_properties(cli_move_benchmark_smoke PROPERTIES TIMEOUT 60)

endif()

install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
)

message(STATUS "Project      : ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Compiler     : ${CMAKE_C_COMPILER}")
message(STATUS "Build type   : ${CMAKE_BUILD_TYPE}")
message(STATUS "C standard   : C${CMAKE_C_STANDARD}")
