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
  add_executable(test_hashmap tests/unit/test_hashmap.c src/utils/hashmap.c)
  target_include_directories(test_hashmap PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(test_hashmap PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -UNDEBUG)
  add_test(NAME hashmap_unit COMMAND test_hashmap)

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
  add_test(NAME queue_unit COMMAND test_queue)

  add_executable(test_scanner tests/integration/test_scanner.c src/scanner/scanner.c)
  target_include_directories(test_scanner PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_options(test_scanner PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -UNDEBUG)
  add_test(NAME scanner_security COMMAND test_scanner)

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
  target_compile_options(test_watcher PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -UNDEBUG)
  add_test(NAME watcher_integration COMMAND test_watcher)

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
