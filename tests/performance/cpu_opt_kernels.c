#include "cpu_opt_kernels.h"
#include <string.h>

size_t scalar_count(const unsigned char *data, size_t length,
                           unsigned char needle)
{
    size_t count = 0;
    for (size_t i = 0; i < length; ++i)
        count += (size_t)(data[i] == needle);
    return count;
}

/* Mature libc reference; length-bounded, including embedded NUL bytes. */
size_t libc_count(const unsigned char *data, size_t length,
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

