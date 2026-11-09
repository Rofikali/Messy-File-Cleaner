#include "journal/fcj3_recovery_classifier.h"

#include <errno.h>
#include <string.h>

static int bounded_string_is_nonempty(const char *value, size_t capacity)
{
    const char *terminator;

    if (!value || capacity == 0U)
        return 0;
    terminator = memchr(value, '\0', capacity);
    return terminator != NULL && terminator != value;
}

static int valid_transaction_identity(const Fcj3Transaction *transaction)
{
    const char *operation_id_end;
    const char *component_end;

    if (!bounded_string_is_nonempty(transaction->operation_id,
                                    sizeof(transaction->operation_id)) ||
        !bounded_string_is_nonempty(transaction->source,
                                    sizeof(transaction->source)) ||
        !bounded_string_is_nonempty(transaction->destination,
                                    sizeof(transaction->destination)) ||
        !bounded_string_is_nonempty(transaction->staging_component,
                                    sizeof(transaction->staging_component)))
        return 0;

    operation_id_end = memchr(transaction->operation_id, '\\0',
                              sizeof(transaction->operation_id));
    if (operation_id_end - transaction->operation_id !=
        (ptrdiff_t)FCJ3_OPERATION_ID_HEX_LENGTH)
        return 0;
    for (size_t i = 0; i < FCJ3_OPERATION_ID_HEX_LENGTH; i++)
    {
        unsigned char c = (unsigned char)transaction->operation_id[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
            return 0;
    }

    component_end = memchr(transaction->staging_component, '\\0',
                           sizeof(transaction->staging_component));
    if (component_end - transaction->staging_component <= 8 ||
        strncmp(transaction->staging_component, ".fc-tmp-", 8U) != 0)
        return 0;
    for (const char *cursor = transaction->staging_component;
         cursor < component_end; cursor++)
    {
        unsigned char c = (unsigned char)*cursor;
        if (c == '/' || c < 0x20U || c == 0x7fU)
            return 0;
    }
    return 1;
}

static int valid_state(Fcj3State state)
{
    return state >= FCJ3_STATE_STAGED && state <= FCJ3_STATE_RECOVERED;
}

static int valid_entry_evidence(Fcj3EntryEvidence evidence)
{
    return evidence >= FCJ3_EVIDENCE_ABSENT &&
           evidence <= FCJ3_EVIDENCE_UNKNOWN_ERROR;
}

static int valid_source_evidence(Fcj3SourceEvidence evidence)
{
    return evidence >= FCJ3_SOURCE_ABSENT &&
           evidence <= FCJ3_SOURCE_UNKNOWN_ERROR;
}

static int valid_directory_evidence(Fcj3DirectoryEvidence evidence)
{
    return evidence >= FCJ3_DIRECTORY_IDENTITY_MATCH &&
           evidence <= FCJ3_DIRECTORY_UNKNOWN_ERROR;
}

static int entry_is_present(Fcj3EntryEvidence evidence)
{
    return evidence != FCJ3_EVIDENCE_ABSENT;
}

static void set_decision(Fcj3RecoveryDecision *decision,
                         Fcj3RecoveryClass classification,
                         Fcj3RecoveryReason reason)
{
    decision->classification = classification;
    decision->reason = reason;
}

static int invalid_input(Fcj3RecoveryDecision *decision,
                         Fcj3RecoveryReason reason)
{
    if (decision)
        set_decision(decision, FCJ3_CLASS_INVALID_INPUT, reason);
    errno = EINVAL;
    return -1;
}

static int manual_review(Fcj3RecoveryDecision *decision,
                         Fcj3RecoveryReason reason)
{
    set_decision(decision, FCJ3_CLASS_MANUAL_REVIEW, reason);
    return 0;
}

int fcj3_recovery_classify(const Fcj3RecoveryEvidence *evidence,
                           Fcj3RecoveryDecision *decision)
{
    const Fcj3Transaction *transaction;

    if (!evidence || !decision)
        return invalid_input(decision, FCJ3_REASON_INVALID_ARGUMENT);

    transaction = evidence->transaction;
    if (!transaction || !valid_transaction_identity(transaction) ||
        !valid_state(transaction->state))
        return invalid_input(decision, FCJ3_REASON_INVALID_TRANSACTION);

    if (!valid_directory_evidence(evidence->directory) ||
        !valid_entry_evidence(evidence->stage) ||
        !valid_entry_evidence(evidence->destination) ||
        !valid_source_evidence(evidence->source))
        return invalid_input(decision, FCJ3_REASON_INVALID_EVIDENCE);

    /* Child observations are not trustworthy unless the parent inode matches. */
    if (evidence->directory == FCJ3_DIRECTORY_IDENTITY_MISMATCH)
        return manual_review(decision,
                             FCJ3_REASON_DIRECTORY_IDENTITY_MISMATCH);
    if (evidence->directory == FCJ3_DIRECTORY_UNKNOWN_ERROR)
        return manual_review(decision, FCJ3_REASON_DIRECTORY_UNKNOWN);

    if (evidence->source == FCJ3_SOURCE_UNKNOWN_ERROR)
        return manual_review(decision, FCJ3_REASON_SOURCE_UNKNOWN);

    if (evidence->stage == FCJ3_EVIDENCE_UNKNOWN_ERROR ||
        evidence->destination == FCJ3_EVIDENCE_UNKNOWN_ERROR)
        return manual_review(decision, FCJ3_REASON_ENTRY_UNKNOWN);

    if (evidence->stage == FCJ3_EVIDENCE_MISMATCHING_REGULAR ||
        evidence->stage == FCJ3_EVIDENCE_SYMLINK ||
        evidence->stage == FCJ3_EVIDENCE_SPECIAL ||
        evidence->destination == FCJ3_EVIDENCE_MISMATCHING_REGULAR ||
        evidence->destination == FCJ3_EVIDENCE_SYMLINK ||
        evidence->destination == FCJ3_EVIDENCE_SPECIAL)
        return manual_review(decision,
                             FCJ3_REASON_ENTRY_TYPE_OR_IDENTITY_MISMATCH);

    if (entry_is_present(evidence->stage) &&
        entry_is_present(evidence->destination))
        return manual_review(decision,
                             FCJ3_REASON_STAGE_AND_DESTINATION_BOTH_PRESENT);

    if ((transaction->state == FCJ3_STATE_STAGED ||
         transaction->state == FCJ3_STATE_STARTED) &&
        evidence->source == FCJ3_SOURCE_ABSENT)
        return manual_review(decision, FCJ3_REASON_SOURCE_MISSING);

    switch (transaction->state)
    {
        case FCJ3_STATE_STAGED:
            if (evidence->destination == FCJ3_EVIDENCE_ABSENT &&
                (evidence->stage == FCJ3_EVIDENCE_ABSENT ||
                 evidence->stage == FCJ3_EVIDENCE_MATCHING_REGULAR) &&
                evidence->source == FCJ3_SOURCE_PRESENT)
            {
                set_decision(decision, FCJ3_CLASS_PRESERVE_INCOMPLETE,
                             FCJ3_REASON_NONE);
                return 0;
            }
            break;

        case FCJ3_STATE_STARTED:
            if (evidence->stage == FCJ3_EVIDENCE_MATCHING_REGULAR &&
                evidence->destination == FCJ3_EVIDENCE_ABSENT &&
                evidence->source == FCJ3_SOURCE_PRESENT)
            {
                set_decision(decision, FCJ3_CLASS_PRESERVE_INCOMPLETE,
                             FCJ3_REASON_NONE);
                return 0;
            }
            if (evidence->stage == FCJ3_EVIDENCE_ABSENT &&
                evidence->destination == FCJ3_EVIDENCE_MATCHING_REGULAR)
            {
                /* The protocol table explicitly permits either source state. */
                set_decision(decision, FCJ3_CLASS_PRESERVE_PUBLISHED,
                             FCJ3_REASON_NONE);
                return 0;
            }
            if (evidence->stage == FCJ3_EVIDENCE_ABSENT &&
                evidence->destination == FCJ3_EVIDENCE_ABSENT &&
                evidence->source == FCJ3_SOURCE_PRESENT)
            {
                set_decision(decision, FCJ3_CLASS_PRESERVE_INCOMPLETE,
                             FCJ3_REASON_NONE);
                return 0;
            }
            break;

        case FCJ3_STATE_PUBLISHED:
            if (evidence->stage == FCJ3_EVIDENCE_ABSENT &&
                evidence->destination == FCJ3_EVIDENCE_MATCHING_REGULAR)
            {
                set_decision(decision, FCJ3_CLASS_PRESERVE_PUBLISHED,
                             FCJ3_REASON_NONE);
                return 0;
            }
            if (evidence->destination == FCJ3_EVIDENCE_ABSENT)
                return manual_review(decision,
                                     FCJ3_REASON_EXPECTED_DESTINATION_MISSING);
            break;

        case FCJ3_STATE_COMPLETED:
            if (evidence->stage == FCJ3_EVIDENCE_ABSENT &&
                evidence->destination == FCJ3_EVIDENCE_MATCHING_REGULAR)
            {
                set_decision(decision, FCJ3_CLASS_NO_ACTION_TERMINAL,
                             FCJ3_REASON_NONE);
                return 0;
            }
            break;

        case FCJ3_STATE_FAILED:
            if (evidence->stage == FCJ3_EVIDENCE_ABSENT &&
                evidence->destination == FCJ3_EVIDENCE_ABSENT)
            {
                set_decision(decision, FCJ3_CLASS_NO_ACTION_TERMINAL,
                             FCJ3_REASON_NONE);
                return 0;
            }
            break;

        case FCJ3_STATE_RECOVERED:
            /*
             * Conservative resolution of the design's "non-conflicting"
             * wording: only an empty stage+destination namespace is terminal.
             * Any visible matching inode needs operator investigation.
             */
            if (evidence->stage == FCJ3_EVIDENCE_ABSENT &&
                evidence->destination == FCJ3_EVIDENCE_ABSENT)
            {
                set_decision(decision, FCJ3_CLASS_NO_ACTION_TERMINAL,
                             FCJ3_REASON_NONE);
                return 0;
            }
            return manual_review(decision,
                                 FCJ3_REASON_UNEXPECTED_VISIBLE_ENTRY);

        case FCJ3_STATE_NONE:
        default:
            return invalid_input(decision, FCJ3_REASON_INVALID_TRANSACTION);
    }

    return manual_review(decision, FCJ3_REASON_STATE_EVIDENCE_CONFLICT);
}
