### ✅ Final Project Structure with cMakelists.txt:
cleaner/
│
├── CMakeLists.txt
├── cmake/
│   └── CompilerWarnings.cmake
│
├── include/
│   └── cleaner/
│       │
│       ├── config.h
│       │
│       ├── core/                    # PURE domain logic (NO OS headers)
│       │   ├── processor.h
│       │   ├── scanner.h
│       │   ├── organizer.h
│       │   └── renamer.h
│       │
│       ├── system/                  # Infrastructure orchestration
│       │   ├── threadpool.h
│       │   ├── logger.h
│       │   └── metrics.h
│       │
│       └── platform/                # OS abstraction layer (ONLY OS contact)
│           ├── filesystem.h
│           ├── threads.h
│           ├── mutex.h
│           ├── atomic.h
│           └── time.h
│
├── src/
│   │
│   ├── core/
│   │   ├── processor.c
│   │   ├── scanner.c
│   │   ├── organizer.c
│   │   └── renamer.c
│   │
│   ├── system/
│   │   ├── threadpool.c
│   │   ├── logger.c
│   │   └── metrics.c
│   │
│   ├── platform/
│   │   │
│   │   ├── filesystem/
│   │   │   ├── filesystem_win.c
│   │   │   └── filesystem_posix.c
│   │   │
│   │   ├── threads/
│   │   │   ├── threads_win.c
│   │   │   └── threads_posix.c
│   │   │
│   │   ├── sync/
│   │   │   ├── mutex_win.c
│   │   │   ├── mutex_posix.c
│   │   │   ├── atomic_win.c
│   │   │   └── atomic_posix.c
│   │   │
│   │   └── time/
│   │       ├── time_win.c
│   │       └── time_posix.c
│   │
│   └── cli/
│       └── main.c
│
├── tests/
│   ├── CMakeLists.txt
│   └── test_scanner.c
│
├── docs/
│   └── architecture.md
│
└── build/
    └── bin/
        └── cleaner.exe


🧠 What You Just Achieved

You now have:

Layer	Windows	Linux	Clean
core	same	same	✅
system	same	same	✅
platform	swapped	swapped	✅


Next Senior-Level Moves

Pick your direction:

🔬 Add GitHub CI (build + test + cppcheck on push)

📦 Add install + packaging (cmake --install)

🧵 Improve threadpool correctness (race detection mindset)

📊 Add benchmark target

🧱 Convert scanner to fully test-driven

core/      → business rules
platform/  → Windows-specific APIs
infra/     → threadpool, logging, metrics


cd ..
rmdir /S /Q build
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build . -j
cmake --build . --target static_analysis
ctest