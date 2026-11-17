#ifndef FCJ4_PARSER_TEST_SHIM_H
#define FCJ4_PARSER_TEST_SHIM_H

/* RUST-01A: private C test-support seam, NOT a public/product ABI.
 * Compiled only into test_fcj4_parser_shim; never install or include
 * in production FC_SOURCES. No filesystem, journal I/O or Rust linkage.
 */
#include <stddef.h>

/*
 * C-owned, single-call FCJ4 parser lifetime. All input bytes are borrowed.
 * Return 0: valid FCJ4 buffer; outputs: errno=0, parsed counts.
 * Return -1: parser rejected buffer; errno output is the parser's
 *            immediately captured errno, counts are zero.
 * Return -2: invalid shim arguments, errno output=EINVAL when provided,
 *            count outputs=0 when provided.
 * No ownership, C replay pointers or allocation are exposed.
 */
int fcj4_test_parse_bytes(const unsigned char *bytes, size_t length,
                          int *out_errno, size_t *out_operations,
                          size_t *out_records);

#endif
