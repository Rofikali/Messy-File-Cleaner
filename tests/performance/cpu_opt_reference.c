/* Independent, bounded byte-count reference; not production classification. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t scalar_count(const unsigned char *data, size_t length,
                           unsigned char needle)
{
    size_t count = 0;
    for (size_t i = 0; i < length; ++i)
        count += (size_t)(data[i] == needle);
    return count;
}

/* Mature libc reference; length-bounded, including embedded NUL bytes. */
static size_t libc_count(const unsigned char *data, size_t length,
                         unsigned char needle)
{
    size_t count = 0;
    while (length != 0)
    {
        const unsigned char *hit = memchr(data, needle, length);
        if (!hit)
            break;
        size_t consumed = (size_t)(hit - data) + 1U;
        ++count;
        data += consumed;
        length -= consumed;
    }
    return count;
}

static unsigned char fixture(size_t i, unsigned int pattern)
{
    if (pattern == 0U)
        return (unsigned char)'/';
    if (pattern == 1U)
        return (unsigned char)'.';
    if (pattern == 2U)
        return (unsigned char)(i % 256U);
    return (unsigned char)((i * 73U + 19U) % 256U);
}

int main(void)
{
    const unsigned char needles[] = {0, '/', '.', 0x80, 0xff};
    size_t cases = 0;
    if (scalar_count(NULL, 0, 0) != 0 || libc_count(NULL, 0, 0) != 0)
        return 1;
    for (size_t offset = 0; offset < 32U; ++offset)
        for (size_t length = 0; length <= 257U; ++length)
        {
            size_t bytes = offset + length;
            unsigned char *storage = malloc(bytes ? bytes : 1U);
            if (!storage)
                return 1;
            memset(storage, 0xa5, bytes ? bytes : 1U);
            unsigned char *data = storage + offset;
            for (unsigned int pattern = 0; pattern < 4U; ++pattern)
            {
                for (size_t i = 0; i < length; ++i)
                    data[i] = fixture(i, pattern);
                for (size_t n = 0; n < sizeof needles / sizeof needles[0]; ++n)
                {
                    size_t expected = libc_count(data, length, needles[n]);
                    size_t actual = scalar_count(data, length, needles[n]);
                    if (actual != expected ||
                        (pattern == 0U && expected != (needles[n] == '/' ? length : 0U)) ||
                        (pattern == 1U && expected != (needles[n] == '.' ? length : 0U)))
                    {
                        free(storage);
                        return 1;
                    }
                    ++cases;
                }
            }
            free(storage);
        }
    printf("cpu_opt_reference=PASS cases=%zu offsets=32 lengths=258 patterns=4 needles=5\n", cases);
    return cases == 32U * 258U * 4U * 5U ? 0 : 1;
}
