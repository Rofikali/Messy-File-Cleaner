#include "journal/fcj3_sha256.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void digest_to_hex(const unsigned char digest[32], char output[65])
{
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < 32U; i++)
    {
        output[i * 2U] = hex[digest[i] >> 4];
        output[i * 2U + 1U] = hex[digest[i] & 0x0fU];
    }
    output[64] = '\0';
}

static void expect_digest(const void *data, size_t length,
                          const char *expected_hex, size_t split)
{
    Fcj3Sha256 context;
    unsigned char digest[32];
    char actual_hex[65];

    fcj3_sha256_init(&context);
    if (split > length)
        split = length;
    fcj3_sha256_update(&context, data, split);
    fcj3_sha256_update(&context, (const unsigned char *)data + split,
                       length - split);
    fcj3_sha256_final(&context, digest);
    digest_to_hex(digest, actual_hex);
    assert(strcmp(actual_hex, expected_hex) == 0);
}

int main(void)
{
    static const char long_vector[] =
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

    expect_digest("", 0U,
        "e3b0c44298fc1c149afbf4c8996fb924"
        "27ae41e4649b934ca495991b7852b855", 0U);
    expect_digest("abc", 3U,
        "ba7816bf8f01cfea414140de5dae2223"
        "b00361a396177a9cb410ff61f20015ad", 1U);
    expect_digest(long_vector, sizeof(long_vector) - 1U,
        "248d6a61d20638b8e5c026930c3e6039"
        "a33ce45964ff2167f6ecedd419db06c1", 17U);

    puts("FCJ3 SHA-256 tests passed");
    return 0;
}
