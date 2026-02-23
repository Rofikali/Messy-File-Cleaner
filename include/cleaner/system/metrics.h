// cleaner/include\metrics.h

#ifndef CLEANER_METRICS_H
#define CLEANER_METRICS_H

#include <stdint.h>

/* Lifecycle */
void metrics_init(void);
void metrics_finish(void);

/* Counters */
void metrics_increment_processed(void);
void metrics_increment_moved(void);
void metrics_increment_error(void);

/* Getters */
uint64_t metrics_get_files_processed(void);
uint64_t metrics_get_files_moved(void);
uint64_t metrics_get_errors(void);

uint64_t metrics_get_elapsed_ns(void);
double metrics_get_elapsed_seconds(void);

#endif