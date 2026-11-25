#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "classifier/classifier.h"

int main(void)
{
    HashEntry entries[16];
    HashMap map;
    Classifier c;
    hashmap_init(&map, entries, 16);
    assert(hashmap_put(&map, ".jpg", "Images/") == 0);
    assert(hashmap_put(&map, ".gz", "Archives/") == 0);
    assert(hashmap_put(&map, ".profile", "Profiles/") == 0);
    assert(hashmap_put(&map, ".x/plain", "Wrong/") == 0);
    assert(hashmap_put(&map, ".", "Wrong/") == 0);
    classifier_init(&c, &map, "Others/");

    classifier_init(NULL, &map, "Others/");
    assert(classify(NULL, "photo.jpg") == NULL);
    assert(classify(&c, NULL) == NULL);
    assert(classify(&c, "") == NULL);
    assert(classify(NULL, NULL) == NULL);
    assert(map.size == 5);

    assert(strcmp(classify(&c, "photo.jpg"), "Images/") == 0);
    assert(strcmp(classify(&c, "/tmp/dir.x/photo.jpg"), "Images/") == 0);
    assert(strcmp(classify(&c, "/tmp/dir.x/plain"), "Others/") == 0);
    assert(strcmp(classify(&c, "dir/.profile"), "Others/") == 0);
    assert(strcmp(classify(&c, ".profile"), "Others/") == 0);
    assert(strcmp(classify(&c, "dir/.archive.gz"), "Archives/") == 0);
    assert(strcmp(classify(&c, "archive.tar.gz"), "Archives/") == 0);
    assert(strcmp(classify(&c, "dir/name."), "Others/") == 0);
    assert(strcmp(classify(&c, "dir/"), "Others/") == 0);
    assert(strcmp(classify(&c, "photo.JPG"), "Others/") == 0);
    assert(map.size == 5);
    puts("classifier tests passed");
    return 0;
}
