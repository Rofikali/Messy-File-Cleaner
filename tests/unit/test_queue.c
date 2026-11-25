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

    test_concurrent_fifo(&queue);
    puts("queue tests passed (single producer / single consumer)");
    return 0;
}
