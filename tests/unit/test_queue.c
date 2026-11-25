#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>

#include "core/lf_queue.h"

static FileEvent make_event(int index)
{
    FileEvent event;
    memset(&event, 0, sizeof(event));
    (void)snprintf(event.path, sizeof(event.path), "fixture-%d.dat", index);
    event.type = FILE_CREATED;
    return event;
}

enum { CONCURRENT_EVENTS = QUEUE_SIZE * 8 + 37 };

static void *consume_events(void *arg)
{
    LFQueue *queue = arg;
    for (int i = 0; i < CONCURRENT_EVENTS; ++i)
    {
        FileEvent event;
        while (!lf_dequeue(queue, &event))
            (void)sched_yield();
        FileEvent expected = make_event(i);
        assert(strcmp(event.path, expected.path) == 0);
        assert(event.type == expected.type);
    }
    return NULL;
}

static void test_concurrent_fifo(LFQueue *queue)
{
    lf_init(queue);
    pthread_t consumer;
    assert(pthread_create(&consumer, NULL, consume_events, queue) == 0);
    for (int i = 0; i < CONCURRENT_EVENTS; ++i)
    {
        FileEvent input = make_event(i);
        while (!lf_enqueue(queue, &input))
            (void)sched_yield();
    }
    assert(pthread_join(consumer, NULL) == 0);
    assert(lf_is_empty(queue));
    FileEvent unused;
    assert(!lf_dequeue(queue, &unused));
}

/* Independent user-space regression: keep a nearly full ring while
 * recycling slots across multiple wraparounds. Compare all path bytes,
 * including bytes following the first NUL, and both event types. */
static FileEvent patterned_event(int index)
{
    FileEvent event = make_event(index);
    size_t length = strlen(event.path);
    for (size_t i = length + 1; i < sizeof(event.path); ++i)
        event.path[i] = (char)('A' + (index + (int)i) % 26);
    event.type = index % 2 ? FILE_FOUND : FILE_CREATED;
    return event;
}

static void test_full_ring_recycling(LFQueue *queue)
{
    const int capacity = QUEUE_SIZE - 1;
    const int cycles = QUEUE_SIZE * 3 + 17;
    int produced = 0, consumed = 0;
    lf_init(queue);
    for (; produced < capacity; ++produced)
    {
        FileEvent input = patterned_event(produced);
        assert(lf_enqueue(queue, &input));
    }
    for (int i = 0; i < cycles; ++i)
    {
        FileEvent input = patterned_event(produced);
        assert(!lf_enqueue(queue, &input));
        FileEvent expected = patterned_event(consumed++);
        FileEvent output;
        assert(lf_dequeue(queue, &output));
        assert(memcmp(output.path, expected.path, sizeof(output.path)) == 0);
        assert(output.type == expected.type);
        assert(lf_enqueue(queue, &input));
        ++produced;
    }
    while (consumed < produced)
    {
        FileEvent expected = patterned_event(consumed++);
        FileEvent output;
        assert(lf_dequeue(queue, &output));
        assert(memcmp(output.path, expected.path, sizeof(output.path)) == 0);
        assert(output.type == expected.type);
    }
    FileEvent sentinel = patterned_event(42);
    FileEvent output = sentinel;
    assert(!lf_dequeue(queue, &output));
    assert(memcmp(output.path, sentinel.path, sizeof(output.path)) == 0);
    assert(output.type == sentinel.type);
    assert(lf_is_empty(queue));
}

int main(void)
{
    LFQueue queue;
    FileEvent event;
    lf_init(&queue);

    /* Invalid arguments must not consume or publish an event. */
    lf_init(NULL);
    assert(lf_is_empty(NULL));
    FileEvent input = make_event(42);
    FileEvent output = make_event(99);
    assert(!lf_enqueue(NULL, &input));
    assert(!lf_enqueue(&queue, NULL));
    assert(!lf_dequeue(NULL, &output));
    assert(strcmp(output.path, "fixture-99.dat") == 0);
    assert(lf_is_empty(&queue));
    assert(lf_enqueue(&queue, &input));
    assert(!lf_dequeue(&queue, NULL));
    assert(!lf_enqueue(&queue, NULL));
    assert(!lf_is_empty(&queue));
    assert(lf_dequeue(&queue, &output));
    assert(strcmp(output.path, input.path) == 0);
    assert(output.type == input.type);

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

    test_full_ring_recycling(&queue);
    test_concurrent_fifo(&queue);
    puts("queue tests passed (single producer / single consumer)");
    return 0;
}
