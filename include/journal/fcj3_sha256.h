#ifndef FCJ3_SHA256_H
#define FCJ3_SHA256_H

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint32_t state[8];
    uint64_t bit_count;
    unsigned char block[64];
    size_t block_length;
} Fcj3Sha256;

void fcj3_sha256_init(Fcj3Sha256 *context);
void fcj3_sha256_update(Fcj3Sha256 *context, const void *data, size_t length);
void fcj3_sha256_final(Fcj3Sha256 *context, unsigned char digest[32]);

#endif
