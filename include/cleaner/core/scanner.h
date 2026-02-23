
// cleaner/include/scanner.h
#ifndef CLEANER_CORE_SCANNER_H
#define CLEANER_CORE_SCANNER_H

#include "cleaner/platform/filesystem.h"

typedef int (*cleaner_scan_callback)(
    const cleaner_fs_entry *entry,
    void *user_data);

/* Scans directory and forwards filesystem events */
int cleaner_scan(const char *root_path,
                 cleaner_scan_callback callback,
                 void *user_data);

#endif