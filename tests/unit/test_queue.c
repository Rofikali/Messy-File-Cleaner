#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "core/lf_queue.h"

static FileEvent make_event(int index)
{
    FileEvent event;
    memset(&event, 0, sizeof(event));
    (void)snprintf(event.path, sizeof(event.path), "fixture-%d.dat", index);
    event.type = FILE_CREATED;
    return event;
}

int main(void)
{
    LFQueue queue;
    FileEvent event;
    lf_init(&queue);

    assert(lf_is_empty(&queue));
    assert(!lf_dequeue(&queue, &event));

    for (int i = 0; i < QUEUE_SIZE - 1; i++)
    {
        FileEvent input = make_event(i);
        assert(lf_enqueue(&queue, &input));
    }

    assert(!lf_enqueue(&queue, &(FileEvent){0}));
    assert(!lf_is_empty(&queue));

    for (int i = 0; i < QUEUE_SIZE - 1; i++)
    {
        char expected[sizeof(event.path)];
        (void)snprintf(expected, sizeof(expected), "fixture-%d.dat", i);
        assert(lf_dequeue(&queue, &event));
        assert(strcmp(event.path, expected) == 0);
        assert(event.type == FILE_CREATED);
    }

    assert(lf_is_empty(&queue));
    assert(!lf_dequeue(&queue, &event));

    for (int i = 0; i < 32; i++)
    {
        FileEvent input = make_event(i + 1000);
        assert(lf_enqueue(&queue, &input));
        assert(lf_dequeue(&queue, &event));
        assert(strcmp(event.path, input.path) == 0);
    }

    puts("queue tests passed (single producer / single consumer)");
    return 0;
}
