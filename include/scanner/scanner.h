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
 */
int scan_directory(const char *path, event_handler_fn handler,
                   const atomic_int *stop_signal);

#endif
