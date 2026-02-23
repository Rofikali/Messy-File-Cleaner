// cleaner/include\renamer.h

// #ifndef RENAMER_H
// #define RENAMER_H

// #include <wchar.h>

// void build_timestamped_name(
//     const char*original_name,
//     char*output,
//     size_t size);

// #endif

#ifndef RENAMER_H
#define RENAMER_H

#include <wchar.h>

void renamer_init();
void build_timestamped_name(const char*original_name,
                            const char*extension,
                            char*output,
                            size_t size);

#endif
