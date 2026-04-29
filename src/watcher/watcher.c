#include <sys/inotify.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "core/event.h"
#include "core/queue.h"
#include "watcher/watcher.h"

#define EVENT_BUF_LEN (1024 * (sizeof(struct inotify_event) + 16))

extern EventQueue queue;

// static void enqueue_path(const char *path)
// {
//     FileEvent e;
//     strncpy(e.path, path, PATH_MAX);
//     e.type = FILE_CREATED;

//     enqueue(&queue, &e);
// }
static void enqueue_path(const char *path)
{
    FileEvent e;
    // Use sizeof(e.path) to ensure you never write past the end of the struct member
    strncpy(e.path, path, sizeof(e.path) - 1);
    e.path[sizeof(e.path) - 1] = '\0'; // Always null-terminate
    e.type = FILE_CREATED;

    enqueue(&queue, &e);
}

void start_watcher(const char *path)
{
    int fd = inotify_init();
    if (fd < 0)
    {
        perror("inotify_init");
        return;
    }

    int wd = inotify_add_watch(fd, path, IN_CREATE | IN_MOVED_TO);
    if (wd < 0)
    {
        perror("inotify_add_watch");
        close(fd);
        return;
    }

    char buffer[EVENT_BUF_LEN];

    while (1)
    {
        int length = read(fd, buffer, EVENT_BUF_LEN);
        if (length < 0)
            continue;

        int i = 0;
        while (i < length)
        {
            struct inotify_event *event =
                (struct inotify_event *)&buffer[i];

            if (event->len)
            {
                if (!(event->mask & IN_ISDIR))
                {
                    char full_path[PATH_MAX];
                    snprintf(full_path, PATH_MAX, "%s/%s", path, event->name);

                    enqueue_path(full_path);
                }
            }

            i += sizeof(struct inotify_event) + event->len;
        }
    }
}