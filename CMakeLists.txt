cmake_minimum_required(VERSION 3.20)

# ── Project ────────────────────────────────────────────────────
project(file-cleaner
  VERSION 1.0.0
  DESCRIPTION "Messy File Cleaner"
  LANGUAGES C ASM
)

# ── Fedora 44 / GCC 16 toolchain settings ──────────────────────
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ── Output directories ─────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# ── Compiler flags ─────────────────────────────────────────────
# REMOVE -fhardened from DEBUG_FLAGS:
set(DEBUG_FLAGS
  -O0
  -g3
  -fsanitize=address,undefined
  -fno-omit-frame-pointer
)

# ADD -fhardened only to RELEASE_FLAGS where -O2/-O3 is active:
set(RELEASE_FLAGS
  -O3
  -march=native
  -flto
  -fomit-frame-pointer
  -fhardened
  -DNDEBUG
)

set(DEBUG_FLAGS
  -O0
  -g3
  -fsanitize=address,undefined
  -fno-omit-frame-pointer
)

set(RELEASE_FLAGS
  -O3
  -march=native
  -flto
  -fomit-frame-pointer
  -DNDEBUG
)

set(PROFILE_FLAGS
  -O2
  -g
  -pg
  -fno-omit-frame-pointer
  -DNDEBUG
)

# ── Source collection ──────────────────────────────────────────
file(GLOB_RECURSE C_SOURCES
  ${CMAKE_SOURCE_DIR}/src/*.c
  ${CMAKE_SOURCE_DIR}/src/**/*.c
)

file(GLOB_RECURSE ASM_SOURCES
  ${CMAKE_SOURCE_DIR}/src/*.s
  ${CMAKE_SOURCE_DIR}/src/*.S
  ${CMAKE_SOURCE_DIR}/src/**/*.s
  ${CMAKE_SOURCE_DIR}/src/**/*.S
)

# ── Main executable ────────────────────────────────────────────
add_executable(${PROJECT_NAME}
  ${C_SOURCES}
  ${ASM_SOURCES}
)

# ── Include paths ──────────────────────────────────────────────
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}/include /configs
)

# ── Link libraries ─────────────────────────────────────────────
target_link_libraries(${PROJECT_NAME} PRIVATE
  pthread
  m
)

# ── Apply flags per build type ─────────────────────────────────
target_compile_options(${PROJECT_NAME} PRIVATE ${COMMON_FLAGS})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Build: Debug (ASan + UBSan enabled)")
  target_compile_options(${PROJECT_NAME} PRIVATE ${DEBUG_FLAGS})
  target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address,undefined)

elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "Build: Release (O3 + LTO + march=native)")
  target_compile_options(${PROJECT_NAME} PRIVATE ${RELEASE_FLAGS})
  target_link_options(${PROJECT_NAME} PRIVATE -flto)

elseif(CMAKE_BUILD_TYPE STREQUAL "Profile")
  message(STATUS "Build: Profile (perf + gprof ready)")
  target_compile_options(${PROJECT_NAME} PRIVATE ${PROFILE_FLAGS})
  target_link_options(${PROJECT_NAME} PRIVATE -pg)

else()
  message(STATUS "Build: Default (O2)")
  target_compile_options(${PROJECT_NAME} PRIVATE -O2 -g)
endif()

# ── Assembly: enable listing output in Debug ───────────────────
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -g")
endif()

# ── Install target ─────────────────────────────────────────────
install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
)

# ── Print summary ──────────────────────────────────────────────
message(STATUS "Project   : ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Compiler  : ${CMAKE_C_COMPILER}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Sources   : ${C_SOURCES}")
message(STATUS "ASM files : ${ASM_SOURCES}")
