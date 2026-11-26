#ifndef FC_PUBLIC_API_CANDIDATE_H
#define FC_PUBLIC_API_CANDIDATE_H

/* DESIGN ONLY: test fixture, not an installed header or exported ABI.
 * No implementation; names and layouts may change before approval. */
#include <stdint.h>

typedef struct fc_candidate_engine fc_candidate_engine_t;
typedef int32_t fc_candidate_status_t;

#define FC_CANDIDATE_VERSION UINT32_C(1)
#define FC_CANDIDATE_OK INT32_C(0)
#define FC_CANDIDATE_INVALID INT32_C(1)
#define FC_CANDIDATE_UNSUPPORTED INT32_C(2)
#define FC_CANDIDATE_RESOURCE INT32_C(3)
#define FC_CANDIDATE_IO INT32_C(4)
#define FC_CANDIDATE_BUSY INT32_C(5)

typedef struct {
    uint32_t struct_size;
    uint32_t contract_version;
    const char *root;
    uint64_t root_length;
    const char *config;
    uint64_t config_length;
} fc_candidate_options_t;

typedef struct {
    uint32_t struct_size;
    uint32_t contract_version;
    uint64_t events_seen;
    uint64_t events_enqueued;
    uint64_t previewed_moves;
    uint64_t move_failures;
    uint32_t complete;
    uint32_t reserved;
} fc_candidate_summary_t;

/* Proposed synchronous preview-only contract; see roadmap. */
uint32_t fc_candidate_contract_version(void);
fc_candidate_status_t fc_candidate_create(const fc_candidate_options_t *options,
                                          fc_candidate_engine_t **out_engine);
fc_candidate_status_t fc_candidate_preview(fc_candidate_engine_t *engine,
                                           fc_candidate_summary_t *summary);
void fc_candidate_destroy(fc_candidate_engine_t *engine);

#endif
