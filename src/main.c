#include <stdio.h>
#include "scanner/scanner.h"
#include "utils/hashmap.h"
#include "classifier/classifier.h"

#define MAP_CAP 32

static HashEntry buffer[MAP_CAP];
static HashMap map;
static Classifier classifier;

void init_system()
{
    hashmap_init(&map, buffer, MAP_CAP);

    hashmap_put(&map, ".jpg", "Jpeg/");
    hashmap_put(&map, ".jpeg", "Jpeg/");
    hashmap_put(&map, ".png", "Png/");

    classifier_init(&classifier, &map, "Others/");
}

void handle_event(const char *path)
{
    const char *folder = classify(&classifier, path);

    if (move_file(path, folder) == 0)
    {
        printf("Moved: %s → %s\n", path, folder);
    }
    else
    {
        printf("Failed: %s\n", path);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    init_system();
    scan_directory(argv[1], handle_event);
    return 0;
}
