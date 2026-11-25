# STRING-EVAL-01 — Classifier string-processing baseline

Source baseline 830e4c7340687b52e15441070c5beced02939035. Actual classifier src/classifier/classifier.c uses libc strrchr to find last slash and last dot of final component; preserves case, rejects leading-only dot and trailing dot, falls back for no map match, and rejects null/empty filename. Current 64-slot hashmap unchanged. [GNU C Library](https://www.gnu.org/software/libc/) is reference only, not imported or source-reviewed. Existing classifier and hashmap tests remain authoritative.

First independent test-only comparison links actual classifier/hashmap and a reverse-scan candidate starting after strlen; candidate scans backward until the last dot or slash, using same map and fallback. Not a one-pass whole-string algorithm (strlen precedes reverse scan). Native string-extension-benchmark uses 12 explicit path classes, 0/32/256/1024/4000-byte synthetic prefixes plus slash, dotted directories and varied names. Include mapped extensions, multiple dots, hidden name, hidden name with mapped extension, trailing dot, unknown extension, uppercase, trailing slash, and no extension. Separately check bare names and null/empty input. Same four mappings, 64 slots, same fixtures and insertion order, expected results independently specified; verify every result, per-case counts and unchanged map size. Three rounds per mode/prefix, alternating mode order, 30 samples x 32768 = 983040 classifications. CLOCK_MONOTONIC region includes parsing, map lookup and validation, excludes setup. Not real filesystem path access, representative frequency, standalone strlen/strrchr microbenchmark, libc replacement or end-to-end speed. No timing threshold. CTest timeout 15 seconds. Reproduce Release CMake BUILD_TESTING=ON, build string-extension-benchmark, ctest -R '^string_extension_benchmark_smoke$' -V. Local execution NOT RUN at creation; exact-head CI required.

Retain production classifier, hash map, API/ABI, ownership and libc. No external imports, dependencies, dispatch, spend or release; FS-01B/REL-01 and license lock unchanged. L1–L7/security/Linux/SRE review bounded semantics and fixture cost; networking N/A; CA/MBA/management avoid integration cost without representative evidence. Revert rollback.


## Exhaustive byte strings and extension boundaries — second PR

Baseline 44b6256235d5a75e9091242db0c4f768b38c935c; PR #218 merged, 14/14 exact-head jobs passed (CMake 45662675251, Ubuntu 45662675205, Docker 45662675369, Makefile 45662675246, FCJ4 TSan 45662675402). C17 Release job 139376407382 reports string_extension_benchmark_smoke PASS and 61/61 tests passed. Successful smoke log does not expose individual timings; integrated-main CI NOT VERIFIED (no merge-SHA runs returned).

Extend existing string-extension-benchmark with 299,615 untimed cases: all 299,593 strings of length 0–6 over an eight-byte alphabet (slash, dot, a/j/p/g/z, 0xff); five unknown extension lengths 1, MAX_KEY_LEN−1, MAX_KEY_LEN, MAX_KEY_LEN+1 and MAX_PATH−3; sixteen explicit cases with backslash, whitespace, newline, UTF-8 bytes, high byte, repeated slash/dots and mapped/unmapped endings; one NULL case. Independent forward reference resets last-dot state on slash and compares explicit mapping strings without calling hashmap. Both actual and candidate results must match reference, preserve identical borrowed result pointer and unchanged map size. This is bounded byte-string equivalence, not arbitrary-input proof, Unicode normalization, filesystem validation, map mutation/fault injection or timing evidence. Existing 983,040 timed classifications and 15-second CTest smoke unchanged. Local execution NOT RUN at creation; exact-head CI required. No production algorithm, API/ABI, dependencies, imports, spend, dispatch or release. FS-01B/REL-01/license lock unchanged. Architecture/security/network/business production scope N/A; bounded test cost and revert rollback.


## Exact-source standalone evidence — third PR

Baseline 28938afeaaa218e2f808af3afabcd0f815b20840; PR #219 merged, 13/13 returned exact-head jobs passed (CMake 45663320456, Ubuntu 45663320432, Docker 45663320390, Makefile 45663320452). C17 Release job 139378691696 reports string_extension_benchmark_smoke PASS and 61/61 tests passed. FCJ4 TSan and integrated-main CI NOT VERIFIED (no runs returned). CI success log does not expose individual samples.

Local standalone reproduction in disposable Linux 6.18.57 x86_64 environment (AMD EPYC 9V74, GCC Debian 14.2.0-19, glibc 2.41). Direct Git remote DNS failed; reconstructed five files from connector text and verified each Git blob with git hash-object before compiling. Exact blob hashes: src/classifier/classifier.c 38ac2d056396ffac2c820d7142e3e7975c68341c; include/classifier/classifier.h 7cd3fb1db8a2c95e01102dacee52d475ab741afd; src/utils/hashmap.c 51980e4b33677fe244c4b51bf20418164b767312; include/utils/hashmap.h c7feefb8e774fb25230d7821f97e149c9912a16f; tests/performance/string_extension_benchmark.c 1b156e23e1a60eabf5b7c145ec738dfa024bfff1.

Standalone builds C11/C17/C23 all PASS using cc -std=<standard> -O3 -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes -Werror -fstack-protector-strong -D_FORTIFY_SOURCE=2 -Iinclude tests/performance/string_extension_benchmark.c src/classifier/classifier.c src/utils/hashmap.c -o bench -Wl,-z,relro,-z,now. Separate C17 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer with same sources/include, ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 and UBSAN_OPTIONS=halt_on_error=1 PASS. Four complete executions, each 299,615 untimed boundary cases and 983,040 timed classifications; 3,932,160 timed classifications across builds. Sanitizer timings excluded from comparison. Local full checkout/CMake/CTest NOT RUN.

C17 Release all samples (ns/classification; run 1/2/3), same fixed fixtures and map:

| Prefix bytes | libc samples | Reverse samples | libc median | Reverse median |
|---:|---|---|---:|---:|
| 0 | 16.004/14.843/15.225 | 15.942/15.212/12.610 | 15.225 | 15.212 |
| 32 | 15.809/16.236/15.251 | 12.823/12.816/13.276 | 15.809 | 12.823 |
| 256 | 17.039/17.413/17.498 | 15.659/16.369/15.243 | 17.413 | 15.659 |
| 1024 | 33.554/34.693/33.519 | 21.896/23.453/22.839 | 33.554 | 22.839 |
| 4000 | 78.652/79.905/80.177 | 47.354/47.014/47.177 | 79.905 | 47.177 |

Longest synthetic prefix approximately 1.69x faster (41% less time); shortest effectively tied, overlapping samples. No CPU pinning, clock-frequency/cache controls, independent hosts, real path-frequency distribution or statistical significance claim. Includes map lookup and result verification; not isolated libc or scanner throughput. Retain production classifier and map. This PR records evidence only; no code, ABI, dependency, import, dispatch, spending or release change. Future scoped decision after exact-head CI: close evaluation early if no meaningful safe gap remains. FS-01B/REL-01/license lock unchanged; rollback by revert.


## Scoped evaluation closure — 25 November 2026

PR #220 merged at 63cfa8af42a4e5407fed039b3b1c2bf3aedf0c2a; exact head a421c307d31b3b167e675f85b57847e37d6ef033. All 13 returned exact-head jobs passed (CMake 45664141374, Ubuntu 45664141514, Docker 45664141415, Makefile 45664141397). FCJ4 TSan and integrated-main CI NOT VERIFIED (no corresponding runs returned). Standalone evidence above retained; scope DONE 3/5. Retain production libc-based classifier, no representative throughput or target-host claim. Do not consume additional PR slots solely for polishing. Next independent LINUX-SCANNER-EVAL-01, not a reset of BENCH-SCALE-01 or SCAN-ROBUST-01.
