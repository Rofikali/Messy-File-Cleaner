cmake_minimum_required(VERSION 3.20)

project(file-cleaner
  VERSION 1.0.0
  DESCRIPTION "Messy File Cleaner"
  LANGUAGES C
)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

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

install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
)

message(STATUS "Project   : ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Compiler  : ${CMAKE_C_COMPILER}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
