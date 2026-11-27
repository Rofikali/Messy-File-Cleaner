#ifndef FC_SHARED_CLASSIFIER_REFERENCE_H
#define FC_SHARED_CLASSIFIER_REFERENCE_H

/* Private test-only adapter, not an installed or supported ABI.
 * Input must be a live NUL-terminated string or NULL; no pointer retained.
 * Returns 1 for .jpg, 0 for fallback, -1 for invalid input/setup failure.
 */
int fc_shared_reference_route(const char *filename);

#endif
