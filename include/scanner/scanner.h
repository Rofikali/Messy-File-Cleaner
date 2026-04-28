#ifndef SCANNER_H
#define SCANNER_H

typedef void (*event_handler_fn)(const char *path);

void scan_directory(const char *path, event_handler_fn handler);

#endif
