#include "journal/fcj3_recovery_classifier.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define CHECK_CLASSIFIER_CALL(expression, expected) do { \\
    int actual_result_ = (expression); \\
    assert(actual_result_ == (expected)); \\
} while (0)

static Fcj3Transaction make_transaction(Fcj3State state)
{
    Fcj3Transaction transaction;
    memset(&transaction, 0, sizeof(transaction));
    (void)snprintf(transaction.operation_id, sizeof(transaction.operation_id),
                   "%s", "0123456789abcdef0123456789abcdef");
    (void)snprintf(transaction.source, sizeof(transaction.source), "%s",
                   "/source/file");
    (void)snprintf(transaction.destination, sizeof(transaction.destination), "%s",
                   "/destination/file");
    (void)snprintf(transaction.staging_component,
                   sizeof(transaction.staging_component), "%s", ".fc-tmp-test");
    transaction.state = state;
    return transaction;
}

static Fcj3RecoveryDecision classify(Fcj3Transaction *transaction,
                                     Fcj3EntryEvidence stage,
                                     Fcj3EntryEvidence destination,
                                     Fcj3SourceEvidence source)
{
    Fcj3RecoveryEvidence evidence = {
        .transaction = transaction,
        .directory = FCJ3_DIRECTORY_IDENTITY_MATCH,
        .stage = stage,
        .destination = destination,
        .source = source
    };
    Fcj3RecoveryDecision decision = {
        .classification = FCJ3_CLASS_INVALID_INPUT,
        .reason = FCJ3_REASON_NONE
    };
    CHECK_CLASSIFIER_CALL(fcj3_recovery_classify(&evidence, &decision), 0);
    return decision;
}

static void expect(Fcj3Transaction *transaction,
                   Fcj3EntryEvidence stage,
                   Fcj3EntryEvidence destination,
                   Fcj3SourceEvidence source,
                   Fcj3RecoveryClass expected_class,
                   Fcj3RecoveryReason expected_reason)
{
    Fcj3RecoveryDecision decision =
        classify(transaction, stage, destination, source);
    assert(decision.classification == expected_class);
    assert(decision.reason == expected_reason);
}

static void test_decision_table(void)
{
    Fcj3Transaction tx;

    tx = make_transaction(FCJ3_STATE_STAGED);
    expect(&tx, FCJ3_EVIDENCE_ABSENT, FCJ3_EVIDENCE_ABSENT,
           FCJ3_SOURCE_PRESENT, FCJ3_CLASS_PRESERVE_INCOMPLETE,
           FCJ3_REASON_NONE);
    expect(&tx, FCJ3_EVIDENCE_MATCHING_REGULAR, FCJ3_EVIDENCE_ABSENT,
           FCJ3_SOURCE_PRESENT, FCJ3_CLASS_PRESERVE_INCOMPLETE,
           FCJ3_REASON_NONE);
    expect(&tx, FCJ3_EVIDENCE_ABSENT, FCJ3_EVIDENCE_MATCHING_REGULAR,
           FCJ3_SOURCE_PRESENT, FCJ3_CLASS_MANUAL_REVIEW,
           FCJ3_REASON_STATE_EVIDENCE_CONFLICT);

    tx = make_transaction(FCJ3_STATE_STARTED);
    expect(&tx, FCJ3_EVIDENCE_MATCHING_REGULAR, FCJ3_EVIDENCE_ABSENT,
           FCJ3_SOURCE_PRESENT, FCJ3_CLASS_PRESERVE_INCOMPLETE,
           FCJ3_REASON_NONE);
    expect(&tx, FCJ3_EVIDENCE_ABSENT, FCJ3_EVIDENCE_MATCHING_REGULAR,
           FCJ3_SOURCE_PRESENT, FCJ3_CLASS_PRESERVE_PUBLISHED,
           FCJ3_REASON_NONE);
    expect(&tx, FCJ3_EVIDENCE_ABSENT, FCJ3_EVIDENCE_MATCHING_REGULAR,
           FCJ3_SOURCE_ABSENT, FCJ3_CLASS_PRESERVE_PUBLISHED,
           FCJ3_REASON_NONE);
    expect(&tx, FCJ3_EVIDENCE_ABSENT, FCJ3_EVIDENCE_ABSENT,
           FCJ3_SOURCE_PRESENT, FCJ3_CLASS_PRESERVE_INCOMPLETE,
           FCJ3_REASON_NONE);

    tx = make_transaction(FCJ3_STATE_PUBLISHED);
    expect(&tx, FCJ3_EVIDENCE_ABSENT, FCJ3_EVIDENCE_MATCHING_REGULAR,
           FCJ3_SOURCE_PRESENT, FCJ3_CLASS_PRESERVE_PUBLISHED,
           FCJ3_REASON_NONE);
    expect(&tx, FCJ3_EVIDENCE_ABSENT, FCJ3_EVIDENCE_MATCHING_REGULAR,
           FCJ3_SOURCE_ABSENT, FCJ3_CLASS_PRESERVE_PUBLISHED,
           FCJ3_REASON_NONE);
    expect(&tx, FCJ3_EVIDENCE_ABSENT, FCJ3_EVIDENCE_ABSENT,
           FCJ3_SOURCE_PRESENT, FCJ3_CLASS_MANUAL_REVIEW,
           FCJ3_REASON_EXPECTED_DESTINATION_MISSING);

    tx = make_transaction(FCJ3_STATE_COMPLETED);
    expect(&tx, FCJ3_EVIDENCE_ABSENT, FCJ3_EVIDENCE_MATCHING_REGULAR,
           FCJ3_SOURCE_PRESENT, FCJ3_CLASS_NO_ACTION_TERMINAL,
           FCJ3_REASON_NONE);

    tx = make_transaction(FCJ3_STATE_FAILED);
    expect(&tx, FCJ3_EVIDENCE_ABSENT, FCJ3_EVIDENCE_ABSENT,
           FCJ3_SOURCE_PRESENT, FCJ3_CLASS_NO_ACTION_TERMINAL,
           FCJ3_REASON_NONE);
    expect(&tx, FCJ3_EVIDENCE_ABSENT, FCJ3_EVIDENCE_ABSENT,
           FCJ3_SOURCE_ABSENT, FCJ3_CLASS_NO_ACTION_TERMINAL,
           FCJ3_REASON_NONE);

    tx = make_transaction(FCJ3_STATE_RECOVERED);
    expect(&tx, FCJ3_EVIDENCE_ABSENT, FCJ3_EVIDENCE_ABSENT,
           FCJ3_SOURCE_PRESENT, FCJ3_CLASS_NO_ACTION_TERMINAL,
           FCJ3_REASON_NONE);
    expect(&tx, FCJ3_EVIDENCE_MATCHING_REGULAR, FCJ3_EVIDENCE_ABSENT,
           FCJ3_SOURCE_PRESENT, FCJ3_CLASS_MANUAL_REVIEW,
           FCJ3_REASON_UNEXPECTED_VISIBLE_ENTRY);
}

static void test_global_safety_overrides(void)
{
    const Fcj3State states[] = {
        FCJ3_STATE_STAGED, FCJ3_STATE_STARTED, FCJ3_STATE_PUBLISHED,
        FCJ3_STATE_COMPLETED, FCJ3_STATE_FAILED, FCJ3_STATE_RECOVERED
    };
    const Fcj3EntryEvidence hostile[] = {
        FCJ3_EVIDENCE_MISMATCHING_REGULAR,
        FCJ3_EVIDENCE_SYMLINK,
        FCJ3_EVIDENCE_SPECIAL,
        FCJ3_EVIDENCE_UNKNOWN_ERROR
    };

    for (size_t s = 0; s < sizeof(states) / sizeof(states[0]); s++)
    {
        Fcj3Transaction tx = make_transaction(states[s]);
        for (size_t h = 0; h < sizeof(hostile) / sizeof(hostile[0]); h++)
        {
            Fcj3RecoveryReason reason =
                hostile[h] == FCJ3_EVIDENCE_UNKNOWN_ERROR
                    ? FCJ3_REASON_ENTRY_UNKNOWN
                    : FCJ3_REASON_ENTRY_TYPE_OR_IDENTITY_MISMATCH;
            expect(&tx, hostile[h], FCJ3_EVIDENCE_ABSENT,
                   FCJ3_SOURCE_PRESENT, FCJ3_CLASS_MANUAL_REVIEW, reason);
            expect(&tx, FCJ3_EVIDENCE_ABSENT, hostile[h],
                   FCJ3_SOURCE_PRESENT, FCJ3_CLASS_MANUAL_REVIEW, reason);
        }

        expect(&tx, FCJ3_EVIDENCE_MATCHING_REGULAR,
               FCJ3_EVIDENCE_MATCHING_REGULAR, FCJ3_SOURCE_PRESENT,
               FCJ3_CLASS_MANUAL_REVIEW,
               FCJ3_REASON_STAGE_AND_DESTINATION_BOTH_PRESENT);
        expect(&tx, FCJ3_EVIDENCE_ABSENT, FCJ3_EVIDENCE_ABSENT,
               FCJ3_SOURCE_UNKNOWN_ERROR, FCJ3_CLASS_MANUAL_REVIEW,
               FCJ3_REASON_SOURCE_UNKNOWN);
    }
}

static void test_directory_and_missing_source_gates(void)
{
    Fcj3Transaction tx = make_transaction(FCJ3_STATE_STARTED);
    Fcj3RecoveryEvidence evidence = {
        .transaction = &tx,
        .directory = FCJ3_DIRECTORY_IDENTITY_MISMATCH,
        .stage = FCJ3_EVIDENCE_MATCHING_REGULAR,
        .destination = FCJ3_EVIDENCE_ABSENT,
        .source = FCJ3_SOURCE_PRESENT
    };
    Fcj3RecoveryDecision decision;

    assert(fcj3_recovery_classify(&evidence, &decision) == 0);
    assert(decision.classification == FCJ3_CLASS_MANUAL_REVIEW);
    assert(decision.reason == FCJ3_REASON_DIRECTORY_IDENTITY_MISMATCH);

    evidence.directory = FCJ3_DIRECTORY_UNKNOWN_ERROR;
    assert(fcj3_recovery_classify(&evidence, &decision) == 0);
    assert(decision.reason == FCJ3_REASON_DIRECTORY_UNKNOWN);

    evidence.directory = FCJ3_DIRECTORY_IDENTITY_MATCH;
    evidence.source = FCJ3_SOURCE_ABSENT;
    evidence.stage = FCJ3_EVIDENCE_ABSENT;
    evidence.destination = FCJ3_EVIDENCE_ABSENT;
    assert(fcj3_recovery_classify(&evidence, &decision) == 0);
    assert(decision.classification == FCJ3_CLASS_MANUAL_REVIEW);
    assert(decision.reason == FCJ3_REASON_SOURCE_MISSING);

    tx.state = FCJ3_STATE_STAGED;
    assert(fcj3_recovery_classify(&evidence, &decision) == 0);
    assert(decision.reason == FCJ3_REASON_SOURCE_MISSING);
}

static void test_invalid_inputs_and_determinism(void)
{
    Fcj3Transaction tx = make_transaction(FCJ3_STATE_FAILED);
    Fcj3RecoveryEvidence evidence = {
        .transaction = &tx,
        .directory = FCJ3_DIRECTORY_IDENTITY_MATCH,
        .stage = FCJ3_EVIDENCE_ABSENT,
        .destination = FCJ3_EVIDENCE_ABSENT,
        .source = FCJ3_SOURCE_PRESENT
    };
    Fcj3RecoveryDecision first, second;

    CHECK_CLASSIFIER_CALL(fcj3_recovery_classify(NULL, &first), -1);
    assert(first.classification == FCJ3_CLASS_INVALID_INPUT);
    CHECK_CLASSIFIER_CALL(fcj3_recovery_classify(&evidence, NULL), -1);

    evidence.directory = (Fcj3DirectoryEvidence)99;
    CHECK_CLASSIFIER_CALL(fcj3_recovery_classify(&evidence, &first), -1);
    assert(first.reason == FCJ3_REASON_INVALID_EVIDENCE);
    evidence.directory = FCJ3_DIRECTORY_IDENTITY_MATCH;
    evidence.stage = (Fcj3EntryEvidence)-1;
    assert(fcj3_recovery_classify(&evidence, &first) == -1);
    assert(first.reason == FCJ3_REASON_INVALID_EVIDENCE);
    evidence.stage = FCJ3_EVIDENCE_ABSENT;
    evidence.source = (Fcj3SourceEvidence)99;
    assert(fcj3_recovery_classify(&evidence, &first) == -1);
    assert(first.reason == FCJ3_REASON_INVALID_EVIDENCE);

    evidence.source = FCJ3_SOURCE_PRESENT;
    tx.state = (Fcj3State)99;
    assert(fcj3_recovery_classify(&evidence, &first) == -1);
    assert(first.reason == FCJ3_REASON_INVALID_TRANSACTION);
    tx = make_transaction(FCJ3_STATE_FAILED);

    tx.operation_id[0] = 'G';
    assert(fcj3_recovery_classify(&evidence, &first) == -1);
    assert(first.reason == FCJ3_REASON_INVALID_TRANSACTION);
    tx = make_transaction(FCJ3_STATE_FAILED);
    tx.staging_component[0] = '/';
    assert(fcj3_recovery_classify(&evidence, &first) == -1);
    assert(first.reason == FCJ3_REASON_INVALID_TRANSACTION);
    tx = make_transaction(FCJ3_STATE_FAILED);

    CHECK_CLASSIFIER_CALL(fcj3_recovery_classify(&evidence, &first), 0);
    CHECK_CLASSIFIER_CALL(fcj3_recovery_classify(&evidence, &second), 0);
    assert(first.classification == second.classification);
    assert(first.reason == second.reason);
}

int main(void)
{
    test_decision_table();
    test_global_safety_overrides();
    test_directory_and_missing_source_gates();
    test_invalid_inputs_and_determinism();

    puts("FCJ3 read-only recovery classifier tests passed");
    return 0;
}
