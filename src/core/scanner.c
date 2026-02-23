
// // cleaner/src/scanner.c

#include "cleaner/core/scanner.h"
#include "cleaner/platform/filesystem.h"

typedef struct
{
    cleaner_scan_callback callback;
    void *user_data;
} scan_context;

static int forward_callback(const cleaner_fs_entry *entry,
                            void *user_data)
{
    scan_context *ctx = (scan_context *)user_data;
    return ctx->callback(entry, ctx->user_data);
}

int cleaner_scan(const char *root_path,
                 cleaner_scan_callback callback,
                 void *user_data)
{
    scan_context ctx = {callback, user_data};

    return cleaner_fs_walk(root_path,
                           forward_callback,
                           &ctx);
}
