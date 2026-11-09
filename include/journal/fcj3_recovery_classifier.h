#ifndef FCJ3_RECOVERY_CLASSIFIER_H
#define FCJ3_RECOVERY_CLASSIFIER_H

#include "journal/fcj3_parser.h"

#include <stddef.h>

/*
 * Pure, read-only FCJ3 recovery classification.
 *
 * This module consumes a parser-validated transaction and observations
 * collected by a future filesystem adapter. It performs no syscalls, owns no
 * resources, and cannot request or perform filesystem/journal mutations.
 */

typedef enum
{
    FCJ3_EVIDENCE_ABSENT = 0,
    FCJ3_EVIDENCE_MATCHING_REGULAR,
    FCJ3_EVIDENCE_MISMATCHING_REGULAR,
    FCJ3_EVIDENCE_SYMLINK,
    FCJ3_EVIDENCE_SPECIAL,
    FCJ3_EVIDENCE_UNKNOWN_ERROR
} Fcj3EntryEvidence;

typedef enum
{
    FCJ3_SOURCE_ABSENT = 0,
    FCJ3_SOURCE_PRESENT,
    FCJ3_SOURCE_UNKNOWN_ERROR
} Fcj3SourceEvidence;

typedef enum
{
    FCJ3_DIRECTORY_IDENTITY_MATCH = 0,
    FCJ3_DIRECTORY_IDENTITY_MISMATCH,
    FCJ3_DIRECTORY_UNKNOWN_ERROR
} Fcj3DirectoryEvidence;

typedef enum
{
    FCJ3_CLASS_NO_ACTION_TERMINAL = 0,
    FCJ3_CLASS_PRESERVE_INCOMPLETE,
    FCJ3_CLASS_PRESERVE_PUBLISHED,
    FCJ3_CLASS_MANUAL_REVIEW,
    FCJ3_CLASS_INVALID_INPUT
} Fcj3RecoveryClass;

typedef enum
{
    FCJ3_REASON_NONE = 0,
    FCJ3_REASON_INVALID_ARGUMENT,
    FCJ3_REASON_INVALID_TRANSACTION,
    FCJ3_REASON_INVALID_EVIDENCE,
    FCJ3_REASON_DIRECTORY_IDENTITY_MISMATCH,
    FCJ3_REASON_DIRECTORY_UNKNOWN,
    FCJ3_REASON_SOURCE_UNKNOWN,
    FCJ3_REASON_ENTRY_UNKNOWN,
    FCJ3_REASON_ENTRY_TYPE_OR_IDENTITY_MISMATCH,
    FCJ3_REASON_STAGE_AND_DESTINATION_BOTH_PRESENT,
    FCJ3_REASON_SOURCE_MISSING,
    FCJ3_REASON_STATE_EVIDENCE_CONFLICT,
    FCJ3_REASON_EXPECTED_DESTINATION_MISSING,
    FCJ3_REASON_UNEXPECTED_VISIBLE_ENTRY
} Fcj3RecoveryReason;

typedef struct
{
    const Fcj3Transaction *transaction;
    Fcj3DirectoryEvidence directory;
    Fcj3EntryEvidence stage;
    Fcj3EntryEvidence destination;
    Fcj3SourceEvidence source;
} Fcj3RecoveryEvidence;

typedef struct
{
    Fcj3RecoveryClass classification;
    Fcj3RecoveryReason reason;
} Fcj3RecoveryDecision;

/*
 * Classify one fully replayed transaction using explicit observations.
 *
 * The caller must provide a transaction from a successfully validated
 * complete FCJ3 replay. This function still validates pointer/string
 * termination and all enum values to protect its public boundary.
 *
 * Returns 0 for a valid classification (including MANUAL_REVIEW), or -1 for
 * invalid input. On invalid input, decision is set to INVALID_INPUT when it
 * is non-NULL. The function is deterministic and has no side effects.
 *
 * A matching inode is evidence of identity only, not content integrity.
 * This API never authorizes source retirement, cleanup, publication, or
 * journal transitions.
 */
int fcj3_recovery_classify(const Fcj3RecoveryEvidence *evidence,
                           Fcj3RecoveryDecision *decision);

#endif
