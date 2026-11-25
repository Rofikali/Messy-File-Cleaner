#ifndef SCANNER_H
#define SCANNER_H

#include <stdatomic.h>

/* Return 0 to accept a file, or -1 to stop traversal and propagate errno. */
typedef int (*event_handler_fn)(const char *path);

/* Visits regular files without following symlinks.
 * Returns 0 on complete success, or -1 on traversal/callback/cancellation.
 * Valid pre-cancelled requests fail with ECANCELED before opening the root.
 * Cancellation is cooperative: checked before root access, during traversal,
 * and before callback handoff. It cannot interrupt a running syscall/callback.
 * The first traversal/callback/cancellation failure is retained in errno;
 * a later cancellation or callback failure still stops traversal.
 */
int scan_directory(const char *path, event_handler_fn handler,
                   const atomic_int *stop_signal);

#ifdef FC_SCANNER_TESTING
/* Single-threaded test only: fail the next entry metadata lookup. */
void fc_test_fail_next_scanner_stat(int error);
#endif

#endif
