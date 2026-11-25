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
