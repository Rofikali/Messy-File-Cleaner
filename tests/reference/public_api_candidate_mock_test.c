/* Test-only caller contract. No real engine, filesystem or ABI implementation. */
#include "public_api_candidate.h"
#include <stddef.h>
#include <stdio.h>

fc_candidate_status_t fc_candidate_example(const char *, uint64_t,
                                           const char *, uint64_t,
                                           fc_candidate_summary_t *);

struct fc_candidate_engine { unsigned token; };
static struct fc_candidate_engine handle = {1};
enum scenario {
    SUCCESS, NULL_OUTPUT, VERSION_MISMATCH, CREATE_FAIL, NULL_HANDLE,
    PREVIEW_FAIL, BAD_SIZE, BAD_VERSION, INCOMPLETE, BAD_RESERVED
};
static enum scenario mode;
static unsigned versions, creates, previews, destroys, invalid_calls;

uint32_t fc_candidate_contract_version(void)
{
    ++versions;
    return mode == VERSION_MISMATCH ? 2u : FC_CANDIDATE_VERSION;
}

fc_candidate_status_t fc_candidate_create(const fc_candidate_options_t *opts,
                                          fc_candidate_engine_t **out)
{
    ++creates;
    if (!opts || !out || *out != NULL ||
        opts->struct_size != sizeof *opts ||
        opts->contract_version != FC_CANDIDATE_VERSION ||
        !opts->root || !opts->config ||
        opts->root_length != 1 || opts->config_length != 1 ||
        opts->root[0] != 'r' || opts->config[0] != 'c') {
        ++invalid_calls;
        return FC_CANDIDATE_INVALID;
    }
    if (mode == CREATE_FAIL)
        return FC_CANDIDATE_RESOURCE;
    if (mode != NULL_HANDLE)
        *out = &handle;
    return FC_CANDIDATE_OK;
}

fc_candidate_status_t fc_candidate_preview(fc_candidate_engine_t *engine,
                                           fc_candidate_summary_t *summary)
{
    ++previews;
    if (engine != &handle || !summary ||
        summary->struct_size != sizeof *summary ||
        summary->contract_version != FC_CANDIDATE_VERSION ||
        summary->complete != 0 || summary->reserved != 0) {
        ++invalid_calls;
        return FC_CANDIDATE_INVALID;
    }
    summary->events_seen = 7;
    summary->events_enqueued = 5;
    summary->previewed_moves = 3;
    summary->move_failures = 0;
    summary->complete = 1;
    switch (mode) {
    case PREVIEW_FAIL: return FC_CANDIDATE_IO;
    case BAD_SIZE: summary->struct_size = 0; break;
    case BAD_VERSION: summary->contract_version = 2; break;
    case INCOMPLETE: summary->complete = 0; break;
    case BAD_RESERVED: summary->reserved = 1; break;
    default: break;
    }
    return FC_CANDIDATE_OK;
}

void fc_candidate_destroy(fc_candidate_engine_t *engine)
{
    ++destroys;
    if (engine != &handle || previews != 1 || destroys != 1)
        ++invalid_calls;
}

static int unchanged(const fc_candidate_summary_t *s)
{
    return s->struct_size == 91 && s->contract_version == 92 &&
           s->events_seen == 93 && s->events_enqueued == 94 &&
           s->previewed_moves == 95 && s->move_failures == 96 &&
           s->complete == 97 && s->reserved == 98;
}

int main(void)
{
    static const struct {
        enum scenario scenario;
        fc_candidate_status_t status;
        unsigned version, create, preview, destroy;
    } cases[] = {
        {SUCCESS, FC_CANDIDATE_OK, 1, 1, 1, 1},
        {NULL_OUTPUT, FC_CANDIDATE_INVALID, 0, 0, 0, 0},
        {VERSION_MISMATCH, FC_CANDIDATE_UNSUPPORTED, 1, 0, 0, 0},
        {CREATE_FAIL, FC_CANDIDATE_RESOURCE, 1, 1, 0, 0},
        {NULL_HANDLE, FC_CANDIDATE_INVALID, 1, 1, 0, 0},
        {PREVIEW_FAIL, FC_CANDIDATE_IO, 1, 1, 1, 1},
        {BAD_SIZE, FC_CANDIDATE_UNSUPPORTED, 1, 1, 1, 1},
        {BAD_VERSION, FC_CANDIDATE_UNSUPPORTED, 1, 1, 1, 1},
        {INCOMPLETE, FC_CANDIDATE_INVALID, 1, 1, 1, 1},
        {BAD_RESERVED, FC_CANDIDATE_INVALID, 1, 1, 1, 1}
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        mode = cases[i].scenario;
        versions = creates = previews = destroys = invalid_calls = 0;
        fc_candidate_summary_t out = {91, 92, 93, 94, 95, 96, 97, 98};
        fc_candidate_status_t status = fc_candidate_example(
            "r", 1, "c", 1, mode == NULL_OUTPUT ? NULL : &out);
        int valid = mode == SUCCESS
            ? out.struct_size == sizeof out &&
              out.contract_version == FC_CANDIDATE_VERSION &&
              out.events_seen == 7 && out.events_enqueued == 5 &&
              out.previewed_moves == 3 && out.move_failures == 0 &&
              out.complete == 1 && out.reserved == 0
            : unchanged(&out);
        if (status != cases[i].status || !valid || invalid_calls ||
            versions != cases[i].version || creates != cases[i].create ||
            previews != cases[i].preview || destroys != cases[i].destroy) {
            fprintf(stderr, "mock consumer case %zu FAILED\n", i);
            return 1;
        }
    }
    puts("public_api_mock_consumer=PASS cases=10");
    return 0;
}
