#ifndef SCANNER_H
#define SCANNER_H

#include <signal.h>

/* Return 0 to accept a file, or -1 to stop traversal and propagate errno. */
typedef int (*event_handler_fn)(const char *path);

/* Visits regular files without following symlinks.
 * Returns 0 on complete success, or -1 on traversal/callback/cancellation.
 * If stop_signal is non-NULL and becomes nonzero, traversal stops with ECANCELED.
 */
int scan_directory(const char *path, event_handler_fn handler,
                   const volatile sig_atomic_t *stop_signal);

#endif
