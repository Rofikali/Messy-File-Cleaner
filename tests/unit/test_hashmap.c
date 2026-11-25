#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "utils/hashmap.h"

int main(void)
{
    HashMap map;
    HashEntry entries[8];

    hashmap_init(&map, entries, 8);
    assert(map.capacity == 8);
    assert(map.size == 0);
    assert(hashmap_get(&map, ".jpg") == NULL);

    assert(hashmap_put(&map, ".jpg", "Jpeg/") == 0);
    assert(map.size == 1);
    assert(strcmp(hashmap_get(&map, ".jpg"), "Jpeg/") == 0);

    assert(hashmap_put(&map, ".jpg", "Images/") == 0);
    assert(map.size == 1);
    assert(strcmp(hashmap_get(&map, ".jpg"), "Images/") == 0);

    /* Updating from a borrowed value must support overlapping source bytes. */
    const char *borrowed = hashmap_get(&map, ".jpg");
    assert(hashmap_put(&map, ".jpg", borrowed) == 0);
    assert(strcmp(hashmap_get(&map, ".jpg"), "Images/") == 0);
    borrowed = hashmap_get(&map, ".jpg");
    assert(hashmap_put(&map, ".jpg", borrowed + 1) == 0);
    assert(strcmp(hashmap_get(&map, ".jpg"), "mages/") == 0);
    assert(map.size == 1);
    assert(hashmap_put(&map, ".jpg", "Images/") == 0);

    assert(hashmap_put(&map, ".png", "Png/") == 0);
    assert(hashmap_put(&map, ".gif", "Gif/") == 0);
    assert(map.size == 3);
    assert(strcmp(hashmap_get(&map, ".png"), "Png/") == 0);
    assert(hashmap_get(&map, ".missing") == NULL);

    assert(hashmap_put(&map, "1234567890123456", "x") == -1);
    assert(hashmap_put(&map, ".bad", "1234567890123456789012345678901234567890123456789012345678901234") == -1);
    assert(map.size == 3);

    HashMap invalid;
    HashEntry invalid_entries[3];
    hashmap_init(&invalid, invalid_entries, 3);
    assert(invalid.capacity == 0);
    assert(hashmap_put(&invalid, ".txt", "Text/") == -1);
    assert(hashmap_get(&invalid, ".txt") == NULL);

    /* Malformed metadata must be rejected without probing or modifying entries. */
    HashMap malformed = map;
    malformed.size = map.capacity + 1;
    assert(hashmap_put(&malformed, ".jpg", "Changed/") == -1);
    assert(hashmap_get(&malformed, ".jpg") == NULL);
    assert(malformed.size == map.capacity + 1);
    assert(strcmp(hashmap_get(&map, ".jpg"), "Images/") == 0);
    assert(map.size == 3);

    /* A power-of-two capacity whose byte extent overflows size_t is invalid. */
    size_t impossible_capacity = SIZE_MAX / 2 + 1;
    malformed = map;
    malformed.capacity = impossible_capacity;
    assert(hashmap_put(&malformed, ".jpg", "Changed/") == -1);
    assert(hashmap_get(&malformed, ".jpg") == NULL);
    assert(strcmp(hashmap_get(&map, ".jpg"), "Images/") == 0);
    hashmap_init(&invalid, entries, impossible_capacity);
    assert(invalid.entries == NULL && invalid.capacity == 0 && invalid.size == 0);
    assert(strcmp(hashmap_get(&map, ".jpg"), "Images/") == 0);

    /* A contradictory full count with unused slots must not mutate storage. */
    malformed = map;
    malformed.size = malformed.capacity;
    unsigned char snapshot[sizeof(entries)];
    memcpy(snapshot, entries, sizeof(snapshot));
    assert(hashmap_put(&malformed, ".missing", "Unexpected/") == -1);
    assert(malformed.size == malformed.capacity);
    assert(memcmp(snapshot, entries, sizeof(snapshot)) == 0);
    assert(hashmap_get(&map, ".missing") == NULL);
    assert(map.size == 3);

    /* A genuinely full map still permits existing-key updates. */
    HashMap full;
    HashEntry full_entries[2];
    hashmap_init(&full, full_entries, 2);
    assert(hashmap_put(&full, ".a", "A/") == 0);
    assert(hashmap_put(&full, ".b", "B/") == 0);
    assert(full.size == full.capacity);
    assert(hashmap_put(&full, ".c", "C/") == -1);
    assert(hashmap_put(&full, ".a", "Updated/") == 0);
    assert(full.size == full.capacity);
    assert(strcmp(hashmap_get(&full, ".a"), "Updated/") == 0);
    assert(strcmp(hashmap_get(&full, ".b"), "B/") == 0);
    assert(hashmap_get(&full, ".c") == NULL);

    puts("hashmap tests passed");
    return 0;
}
