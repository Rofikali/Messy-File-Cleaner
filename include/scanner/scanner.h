#ifndef SCANNER_H
#define SCANNER_H

/* Return 0 to accept a file, or -1 to stop traversal and propagate errno. */
typedef int (*event_handler_fn)(const char *path);

/* Visits regular files without following symlinks.
 * Returns 0 on complete success, or -1 on traversal/callback failure.
 */
int scan_directory(const char *path, event_handler_fn handler);

#endif
