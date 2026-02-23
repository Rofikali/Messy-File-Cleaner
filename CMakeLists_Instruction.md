### 🔥 How To Build (Windows MinGW)

        mkdir build
        cd build
        cmake -G "MinGW Makefiles" ..
        cmake --build . -j


### Unit Testing ( Testing Framework: CTest )
        cmake --build . -j
        ctest --output-on-failure
        ctest

                🧠 Important Engineering Lesson

                CTest runs tests from the build directory, not source directory.

                Never assume relative paths exist.

                Always create test fixtures inside the test itself.

                Professional C projects follow this pattern.
        

### Remove build artifacts:
        rmdir /S /Q build

### 🔥 Debug Build
        cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug ..
        cmake --build .

### 🔥 Release Build
        cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
        cmake --build .

### 🔥 Enable LTO
        cmake -G "MinGW Makefiles" -DENABLE_LTO=ON ..
        cmake --build .

### 🔥 Enable AddressSanitizer
        cmake -G "MinGW Makefiles" -DENABLE_ASAN=ON -DCMAKE_BUILD_TYPE=Debug ..
        cmake --build .

### 🔥 PGO (Correct Workflow)
        Step 1 — Generate profile
        cmake -G "MinGW Makefiles" -DENABLE_PGO_GENERATE=ON ..
        cmake --build .


Run program normally:

build\cleaner.exe "D:\Testing" --recursive

Step 2 — Use profile

Delete build folder completely.

Then:

cmake -G "MinGW Makefiles" -DENABLE_PGO_USE=ON ..
cmake --build .


Done.

🔥 Linux Build

On Linux:

mkdir build
cd build
cmake ..
make -j


No changes needed.

What You Just Gained

✔ Cross-platform without shell hacks
✔ Automatic dependency handling
✔ Clean Debug/Release separation
✔ IDE compatibility
✔ MSVC support instantly
✔ Production-ready configuration

<!-- 
cmake_minimum_required(VERSION 3.16)
project(cleaner VERSION 1.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES
    src/*.c
)

include_directories(include)

add_executable(cleaner ${SOURCES})

if (WIN32)
    target_link_libraries(cleaner PRIVATE
        kernel32
        user32
    )
endif()

target_compile_options(cleaner PRIVATE
    -Wall
    -Wextra
    -O2
) -->
