#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

static int expired(const struct timespec *now, const struct timespec *deadline)
{
    return now->tv_sec > deadline->tv_sec ||
           (now->tv_sec == deadline->tv_sec &&
            now->tv_nsec >= deadline->tv_nsec);
}

int main(void)
{
    int rc = 1, wake = -1;
    int timer = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timer < 0) {
        if (errno == ENOSYS || errno == EPERM || errno == EACCES) {
            puts("NOT VERIFIED: timerfd unavailable or restricted");
            return 77;
        }
        return 1;
    }
    wake = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wake < 0) goto done;
    int status = fcntl(timer, F_GETFL);
    int flags = fcntl(timer, F_GETFD);
    if (status < 0 || flags < 0 || !(status & O_NONBLOCK) ||
        !(flags & FD_CLOEXEC)) goto done;
    struct pollfd p[2] = {
        { .fd = timer, .events = POLLIN },
        { .fd = wake, .events = POLLIN }
    };
    uint64_t count = 0, one = 1;
    if (poll(p, 2, 0) != 0) goto done;
    if (read(timer, &count, sizeof(count)) != -1 || errno != EAGAIN)
        goto done;
    if (write(wake, &one, sizeof(one)) != (ssize_t)sizeof(one) ||
        poll(p, 2, 0) != 1 || p[0].revents != 0 ||
        p[1].revents != POLLIN ||
        read(wake, &count, sizeof(count)) != (ssize_t)sizeof(count) ||
        count != 1) goto done;

    struct itimerspec setting = {0};
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &setting.it_value)) goto done;
    setting.it_value.tv_nsec += 20000000L;
    if (setting.it_value.tv_nsec >= 1000000000L) {
        ++setting.it_value.tv_sec;
        setting.it_value.tv_nsec -= 1000000000L;
    }
    if (timerfd_settime(timer, TFD_TIMER_ABSTIME, &setting, NULL)) goto done;
    if (poll(p, 2, 2000) != 1 || p[0].revents != POLLIN ||
        p[1].revents != 0) goto done;
    if (clock_gettime(CLOCK_MONOTONIC, &now) ||
        !expired(&now, &setting.it_value)) goto done;
    if (read(timer, &count, sizeof(count)) != (ssize_t)sizeof(count) ||
        count != 1 || poll(p, 2, 0) != 0) goto done;
    if (read(timer, &count, sizeof(count)) != -1 || errno != EAGAIN)
        goto done;
    struct itimerspec remaining;
    if (timerfd_gettime(timer, &remaining) ||
        remaining.it_value.tv_sec != 0 || remaining.it_value.tv_nsec != 0 ||
        remaining.it_interval.tv_sec != 0 || remaining.it_interval.tv_nsec != 0)
        goto done;
    rc = 0;
done:
    if (wake >= 0 && close(wake)) rc = 1;
    if (close(timer)) rc = 1;
    if (rc == 0)
        puts("timerfd_initial_empty=PASS eventfd_routing=PASS absolute_deadline=PASS one_shot=PASS cleanup=PASS");
    return rc;
}
