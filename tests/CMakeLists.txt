# tests/CMakeLists.txt

# add_executable(test_scanner
#     test_scanner.c
# )

# target_link_libraries(test_scanner
#     PRIVATE cleaner_core
# )

# add_test(NAME ScannerTest COMMAND test_scanner)

# cmake --build .
# ctest

add_executable(test_scanner test_scanner.c)
target_link_libraries(test_scanner PRIVATE cleaner_core)

add_test(NAME ScannerTest COMMAND test_scanner)