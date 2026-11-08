#ifndef FCJ3_JOURNAL_H
#define FCJ3_JOURNAL_H

#include "journal/fcj3_writer.h"

#include <pthread.h>
#include <sys/types.h>

/*
 * Test-only FCJ3 append journal.
 *
 * The structure is intentionally visible for stack allocation but its fields
 * are private to this module. Do not copy or move an initialized instance.
 * The caller must stop and join every append-capable thread before close.
 */
typedef struct
{
    int fd;
    pthread_mutex_t mutex;
    int initialized;
} Fcj3Journal;

/*
 * Open or create one journal component relative to an already-open protected
 * directory. The file must be owned by expected_owner, regular, single-linked,
 * and exactly owner-readable/writable (0600). A nonblocking exclusive flock
 * is held until close. A newly created directory entry is parent-fsynced.
 *
 * This API is for disposable integration-test journals only. It must never be
 * linked into or called by the production executable before FCJ3 recovery.
 */
int fcj3_journal_open_at(Fcj3Journal *journal, int directory_fd,
                        const char *component, uid_t expected_owner);

/*
 * Append the exact prepared bytes, fsync the journal, then commit the context.
 * Any write/fsync uncertainty poisons the context; no truncation or retry is
 * attempted. The context remains single-owner even though the journal
 * serializes different append calls.
 */
int fcj3_journal_append_transition(Fcj3Journal *journal,
                                   Fcj3OperationContext *context,
                                   const Fcj3PreparedRecord *prepared);

/* All append-capable threads must be stopped before this call. */
int fcj3_journal_close(Fcj3Journal *journal);

#ifdef FC_TESTING
/* Inject a write error after exactly after_bytes have been accepted.
 * SIZE_MAX disables injection. error_code must be nonzero when enabled.
 */
void fcj3_journal_test_fail_write_after(size_t after_bytes, int error_code);
void fcj3_journal_test_fail_fsync(int error_code);
#endif

#endif
