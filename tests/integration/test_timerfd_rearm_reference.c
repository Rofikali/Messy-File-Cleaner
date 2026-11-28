#define _GNU_SOURCE
#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

static int empty(int fd)
{
    struct pollfd p = { .fd = fd, .events = POLLIN };
    uint64_t count;
    if (poll(&p, 1, 0) != 0) return 0;
    return read(fd, &count, sizeof(count)) == -1 && errno == EAGAIN;
}

static int ready(int fd)
{
    struct pollfd p = { .fd = fd, .events = POLLIN };
    int rc;
    do {
        rc = poll(&p, 1, 2000);
    } while (rc < 0 && errno == EINTR);
    return rc == 1 && p.revents == POLLIN;
}

int main(void)
{
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (fd < 0) {
        if (errno == ENOSYS || errno == EPERM || errno == EACCES) {
            puts("NOT VERIFIED: timerfd unavailable or restricted");
            return 77;
        }
        return 1;
    }
    int rc = 1;
    const struct itimerspec off = {0};
    const struct itimerspec one = { .it_value = { .tv_nsec = 20000000L } };
    const struct itimerspec invalid = { .it_value = { .tv_nsec = 1000000000L } };
    /* Wait for an actual unread expiration before cancelling it. */
    if (timerfd_settime(fd, 0, &one, NULL) || !ready(fd)) goto done;
    if (timerfd_settime(fd, 0, &off, NULL) || !empty(fd)) goto done;
    if (timerfd_settime(fd, 0, &invalid, NULL) != -1 || errno != EINVAL ||
        !empty(fd)) goto done;
    struct itimerspec remaining;
    if (timerfd_gettime(fd, &remaining) ||
        remaining.it_value.tv_sec != 0 || remaining.it_value.tv_nsec != 0 ||
        remaining.it_interval.tv_sec != 0 || remaining.it_interval.tv_nsec != 0)
        goto done;

    /* A fresh arm must produce exactly one new expiration, not a stale one. */
    struct itimerspec next = {0};
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &next.it_value)) goto done;
    next.it_value.tv_nsec += 20000000L;
    if (next.it_value.tv_nsec >= 1000000000L) {
        ++next.it_value.tv_sec;
        next.it_value.tv_nsec -= 1000000000L;
    }
    if (timerfd_settime(fd, TFD_TIMER_ABSTIME, &next, NULL) || !ready(fd))
        goto done;
    if (clock_gettime(CLOCK_MONOTONIC, &now) ||
        now.tv_sec < next.it_value.tv_sec ||
        (now.tv_sec == next.it_value.tv_sec &&
         now.tv_nsec < next.it_value.tv_nsec)) goto done;
    uint64_t count;
    if (read(fd, &count, sizeof(count)) != (ssize_t)sizeof(count) ||
        count != 1 || !empty(fd)) goto done;
    rc = 0;
done:
    if (close(fd)) rc = 1;
    if (rc == 0)
        puts("timerfd_unread_disarm=PASS invalid_nsec=PASS rearm_exact=PASS cleanup=PASS");
    return rc;
}
