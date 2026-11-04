#include <assert.h>
#include <stdio.h>
#include <string.h>

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

    puts("hashmap tests passed");
    return 0;
}
