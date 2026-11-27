#include "shared_classifier_reference.h"
#include <stddef.h>
#include <stdio.h>

int main(void)
{
    if (fc_shared_reference_route("fixture/a.jpg") != 1 ||
        fc_shared_reference_route("dir.jpg/no-extension") != 0 ||
        fc_shared_reference_route(".hidden") != 0 ||
        fc_shared_reference_route("a.jpg.") != 0 ||
        fc_shared_reference_route("a.txt") != 0 ||
        fc_shared_reference_route("") != -1 ||
        fc_shared_reference_route(NULL) != -1 ||
        fc_shared_reference_route("second.jpg") != 1)
        return 1;
    puts("shared_classifier_consumer=PASS scenarios=8");
    return 0;
}
