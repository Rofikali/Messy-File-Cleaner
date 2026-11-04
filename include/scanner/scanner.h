#ifndef SCANNER_H
#define SCANNER_H

typedef void (*event_handler_fn)(const char *path);

/* Visits regular files without following symlinks.
 * Returns 0 on success, or -1 when traversal encounters an error.
 */
int scan_directory(const char *path, event_handler_fn handler);

#endif
