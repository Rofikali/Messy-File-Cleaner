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

file(GLOB_RECURSE C_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_executable(${PROJECT_NAME} ${C_SOURCES})

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
  target_compile_options(test_mover PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -UNDEBUG)
  add_test(NAME mover_security COMMAND test_mover)

  add_executable(test_journal
    tests/integration/test_journal.c
    src/journal/journal.c
  )
  target_include_directories(test_journal PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_link_libraries(test_journal PRIVATE Threads::Threads)
  target_compile_options(test_journal PRIVATE -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -UNDEBUG)
  add_test(NAME journal_recovery_security COMMAND test_journal)

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

  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_Interpreter_FOUND)
    add_test(NAME cli_benchmark_smoke
      COMMAND ${Python3_EXECUTABLE}
              "${CMAKE_CURRENT_SOURCE_DIR}/tests/performance/benchmark_cli.py"
              --binary "$<TARGET_FILE:${PROJECT_NAME}>"
              --config "${CMAKE_CURRENT_SOURCE_DIR}/configs/default.conf"
              --files 30
              --runs 3
              --output "${CMAKE_CURRENT_BINARY_DIR}/benchmark-smoke/cli"
    )
    set_tests_properties(cli_benchmark_smoke PROPERTIES TIMEOUT 60)
  endif()

endif()

install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
)

message(STATUS "Project      : ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Compiler     : ${CMAKE_C_COMPILER}")
message(STATUS "Build type   : ${CMAKE_BUILD_TYPE}")
message(STATUS "C standard   : C${CMAKE_C_STANDARD}")
