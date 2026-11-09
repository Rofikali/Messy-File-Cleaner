#ifndef FCJ3_RECOVERY_EVIDENCE_H
#define FCJ3_RECOVERY_EVIDENCE_H

#include "journal/fcj3_recovery_classifier.h"

#include <sys/stat.h>

/*
 * Read-only filesystem evidence adapter for FCJ3 recovery.
 *
 * The caller owns both directory descriptors and must keep them open for the
 * duration of the call. They must already have been resolved by a separately
 * reviewed path-resolution boundary. This module never opens paths or changes
 * filesystem state.
 *
 * The destination parent is verified against the transaction's recorded
 * st_dev/st_ino before stage or destination lookup. FCJ3 does not record source
 * parent identity, so source presence is advisory and cannot prove identity.
 */
typedef struct
{
    Fcj3RecoveryEvidence recovery;
    int directory_errno;
    int source_errno;
    int stage_errno;
    int destination_errno;
    mode_t source_mode;
} Fcj3FilesystemEvidence;

/*
 * Collect observations relative to caller-supplied directory descriptors.
 * source_parent_fd contains transaction->source basename.
 * destination_parent_fd contains destination basename and staging_component.
 *
 * Returns 0 when observations were collected, including ordinary filesystem
 * errors represented by UNKNOWN_ERROR and diagnostic errno fields.
 * Returns -1 for invalid arguments or unsafe/malformed path components.
 * No descriptor is duplicated or closed. No symlink is followed.
 */
int fcj3_recovery_collect_at(const Fcj3Transaction *transaction,
                            int source_parent_fd,
                            int destination_parent_fd,
                            Fcj3FilesystemEvidence *evidence);

#endif
