# # tests/CMakeLists.txt
# cmake_minimum_required(VERSION 3.20)

# project(cleaner C)

# set(CMAKE_C_STANDARD 11)
# set(CMAKE_C_STANDARD_REQUIRED ON)

# # --------------------------------------------------
# # Output directories
# # --------------------------------------------------
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# # --------------------------------------------------
# # Core Library
# # --------------------------------------------------

# # file(GLOB_RECURSE CORE_SOURCES
# #     "${PROJECT_SOURCE_DIR}/src/core/*.c"
# #     "${PROJECT_SOURCE_DIR}/src/system/*.c"
# # )

# # file(GLOB_RECURSE CORE_SOURCES
# #     "${PROJECT_SOURCE_DIR}/src/core/*.c"
# #     "${PROJECT_SOURCE_DIR}/src/system/*.c"
# #     "${PROJECT_SOURCE_DIR}/src/platform/*.c"
# # )
# file(GLOB_RECURSE CORE_SOURCES
#     "${PROJECT_SOURCE_DIR}/src/core/*.c"
#     "${PROJECT_SOURCE_DIR}/src/system/*.c"
# )

# if(WIN32)
#     list(APPEND CORE_SOURCES
#         "${PROJECT_SOURCE_DIR}/src/platform/filesystem_win.c"
#         "${PROJECT_SOURCE_DIR}/src/platform/threads_win.c"
#     )
# else()
#     list(APPEND CORE_SOURCES
#         "${PROJECT_SOURCE_DIR}/src/platform/filesystem_posix.c"
#         "${PROJECT_SOURCE_DIR}/src/platform/threads_posix.c"
#     )
# endif()

# add_library(cleaner_core STATIC ${CORE_SOURCES})

# target_include_directories(cleaner_core
#     PUBLIC ${PROJECT_SOURCE_DIR}/include
# )

# # if (MINGW)
# #     target_compile_options(cleaner_core PRIVATE -Wall -Wextra -Wpedantic)
# # endif()

# if (MINGW)
#     target_link_options(cleaner PRIVATE -mconsole)
# endif()
# # --------------------------------------------------
# # CLI Executable
# # --------------------------------------------------

# add_executable(cleaner
#     src/cli/main.c
# )

# target_link_libraries(cleaner PRIVATE cleaner_core)

# if (MINGW)
#     target_link_options(cleaner PRIVATE -municode -mconsole)
# endif()

# # --------------------------------------------------
# # Testing
# # --------------------------------------------------

# enable_testing()
# add_subdirectory(tests)

# # --------------------------------------------------
# # Static Analysis (Cppcheck)
# # --------------------------------------------------

# find_program(CPPCHECK cppcheck)

# if(CPPCHECK)
#     add_custom_target(
#         static_analysis
#         COMMAND ${CPPCHECK}
#         --enable=all
#         --inconclusive
#         --std=c11
#         ${PROJECT_SOURCE_DIR}/src
#         ${PROJECT_SOURCE_DIR}/include
#     )
# endif()




# # cmake_minimum_required(VERSION 3.20)

# # project(cleaner VERSION 1.0 LANGUAGES C)

# # set(CMAKE_C_STANDARD 11)
# # set(CMAKE_C_STANDARD_REQUIRED ON)

# # # --------------------------------------------------
# # # Output directories
# # # --------------------------------------------------

# # set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# # set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# # # --------------------------------------------------
# # # Core Library
# # # --------------------------------------------------

# # file(GLOB_RECURSE CORE_SOURCES
# #     "${PROJECT_SOURCE_DIR}/src/core/*.c"
# #     "${PROJECT_SOURCE_DIR}/src/system/*.c"
# # )

# # add_library(cleaner_core STATIC ${CORE_SOURCES})

# # target_include_directories(cleaner_core
# #     PUBLIC ${PROJECT_SOURCE_DIR}/include
# # )

# # # --------------------------------------------------
# # # CLI Executable
# # # --------------------------------------------------

# # add_executable(cleaner
# #     src/cli/main.c
# # )

# # target_link_libraries(cleaner PRIVATE cleaner_core)

# # if (MINGW)
# #     target_link_options(cleaner PRIVATE -municode -mconsole)
# # endif()

# # # --------------------------------------------------
# # # Compiler Warnings (Professional Setup)
# # # --------------------------------------------------

# # add_library(project_warnings INTERFACE)

# # if (MSVC)
# #     target_compile_options(project_warnings INTERFACE /W4 /WX)
# # else()
# #     target_compile_options(project_warnings INTERFACE
# #         -Wall
# #         -Wextra
# #         -Wpedantic
# #         -Wconversion
# #         -Wshadow
# #         -Werror
# #     )
# # endif()

# # target_link_libraries(cleaner_core PRIVATE project_warnings)
# # target_link_libraries(cleaner PRIVATE project_warnings)

# # # --------------------------------------------------
# # # Testing
# # # --------------------------------------------------

# # enable_testing()
# # add_subdirectory(tests)

# # # --------------------------------------------------
# # # Static Analysis (Cppcheck)
# # # --------------------------------------------------

# # option(ENABLE_STATIC_ANALYSIS "Enable static analysis" ON)

# # if (ENABLE_STATIC_ANALYSIS)
# #     find_program(CPPCHECK cppcheck)

# #     if (CPPCHECK)
# #         message(STATUS "Cppcheck found: ${CPPCHECK}")
# #         set(CMAKE_C_CPPCHECK
# #             ${CPPCHECK};
# #             --enable=warning,performance,portability;
# #             --inconclusive;
# #             --std=c11;
# #             --quiet
# #         )
# #     endif()
# # endif()

# # # Platform selection
# # if (WIN32)
# #     target_sources(cleaner_core PRIVATE
# #         src/platform/filesystem_win.c
# #         src/platform/threads_win.c
# #     )
# # else()
# #     target_sources(cleaner_core PRIVATE
# #         src/platform/filesystem_posix.c
# #         src/platform/threads_posix.c
# #     )
# # endif()



cmake_minimum_required(VERSION 3.20)

project(cleaner VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --------------------------------------------------
# Output directories
# --------------------------------------------------

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# --------------------------------------------------
# Core Library
# --------------------------------------------------

file(GLOB_RECURSE CORE_SOURCES
    "${PROJECT_SOURCE_DIR}/src/core/*.c"
    "${PROJECT_SOURCE_DIR}/src/system/*.c"
)

add_library(cleaner_core STATIC)

target_sources(cleaner_core PRIVATE ${CORE_SOURCES})

# Platform-specific sources

# if (WIN32)
#     target_sources(cleaner_core PRIVATE
#         src/platform/filesystem_win.c
#         src/platform/threads_win.c
#     )
# else()
#     target_sources(cleaner_core PRIVATE
#         src/platform/filesystem_posix.c
#         src/platform/threads_posix.c
#     )
# endif()
# --------------------------------------------------
# Platform Sources (Attach to cleaner_core)
# --------------------------------------------------

if (WIN32)
    target_sources(cleaner_core PRIVATE
        src/platform/filesystem/filesystem_win.c
        src/platform/threads/threads_win.c
        src/platform/sync/mutex_win.c
        src/platform/sync/atomic_win.c
        src/platform/time/time_win.c
    )
endif()

if (UNIX AND NOT WIN32)
    target_sources(cleaner_core PRIVATE
        src/platform/filesystem_posix.c
        src/platform/threads_posix.c
        src/platform/mutex_posix.c
        src/platform/atomic_posix.c
        src/platform/time_posix.c
    )

    target_link_libraries(cleaner_core PRIVATE pthread)
endif()

target_include_directories(cleaner_core
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

# --------------------------------------------------
# CLI Executable
# --------------------------------------------------

add_executable(cleaner
    src/cli/main.c
)

target_link_libraries(cleaner PRIVATE cleaner_core)

if (MINGW)
    target_link_options(cleaner PRIVATE -mconsole)
endif()

# --------------------------------------------------
# Compiler Warnings
# --------------------------------------------------

add_library(project_warnings INTERFACE)

if (MSVC)
    target_compile_options(project_warnings INTERFACE /W4)
else()
    target_compile_options(project_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
    )
endif()

target_link_libraries(cleaner_core PRIVATE project_warnings)
target_link_libraries(cleaner PRIVATE project_warnings)

# --------------------------------------------------
# Testing
# --------------------------------------------------

enable_testing()
add_subdirectory(tests)