/* Compile-only consumer shape; intentionally never linked or installed. */
#include "public_api_candidate.h"
#include "public_api_candidate.h"
#include <stddef.h>

_Static_assert(sizeof(fc_candidate_status_t) == 4, "fixed status width");
_Static_assert(sizeof(((fc_candidate_summary_t *)0)->events_seen) == 8,
               "fixed counter width");
_Static_assert(offsetof(fc_candidate_options_t, struct_size) == 0,
               "size prefix");
_Static_assert(offsetof(fc_candidate_summary_t, struct_size) == 0,
               "size prefix");
_Static_assert(FC_CANDIDATE_OK != FC_CANDIDATE_INVALID,
               "distinct success and error");

/* Proposed adapter for the existing aggregate batch-preview CLI.
 * This is compile-only: no API implementation or runtime evidence. */
fc_candidate_status_t fc_candidate_example(const char *root, uint64_t root_length,
                                          const char *config, uint64_t config_length,
                                          fc_candidate_summary_t *out_summary)
{
    if (out_summary == NULL)
        return FC_CANDIDATE_INVALID;

    fc_candidate_engine_t *engine = NULL;
    fc_candidate_options_t options = {
        .struct_size = (uint32_t)sizeof options,
        .contract_version = FC_CANDIDATE_VERSION,
        .root = root,
        .root_length = root_length,
        .config = config,
        .config_length = config_length
    };
    fc_candidate_summary_t summary = {
        .struct_size = (uint32_t)sizeof summary,
        .contract_version = FC_CANDIDATE_VERSION
    };
    if (fc_candidate_contract_version() != FC_CANDIDATE_VERSION)
        return FC_CANDIDATE_UNSUPPORTED;

    fc_candidate_status_t status = fc_candidate_create(&options, &engine);
    if (status != FC_CANDIDATE_OK)
        return status;
    if (engine == NULL)
        return FC_CANDIDATE_INVALID;

    status = fc_candidate_preview(engine, &summary);
    fc_candidate_destroy(engine);
    if (status != FC_CANDIDATE_OK)
        return status;

    /* Never publish a partial or incompatible result as a complete report. */
    if (summary.struct_size != (uint32_t)sizeof summary ||
        summary.contract_version != FC_CANDIDATE_VERSION)
        return FC_CANDIDATE_UNSUPPORTED;
    if (summary.complete != UINT32_C(1) || summary.reserved != UINT32_C(0))
        return FC_CANDIDATE_INVALID;

    *out_summary = summary;
    return FC_CANDIDATE_OK;
}
