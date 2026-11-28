#define _GNU_SOURCE
#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

#define PERIOD_NS 20000000L

static struct timespec advance(struct timespec t, long ns)
{
    t.tv_nsec += ns;
    while (t.tv_nsec >= 1000000000L) {
        ++t.tv_sec;
        t.tv_nsec -= 1000000000L;
    }
    return t;
}

/* Called only within this bounded, seconds-long experiment. */
static uint64_t due(struct timespec first, struct timespec now)
{
    int64_t elapsed = (int64_t)(now.tv_sec - first.tv_sec) * INT64_C(1000000000)
                    + (int64_t)now.tv_nsec - first.tv_nsec;
    return elapsed < 0 ? 0 : (uint64_t)(elapsed / PERIOD_NS) + 1;
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
    struct itimerspec setting = { .it_interval = { .tv_nsec = PERIOD_NS } };
    if (clock_gettime(CLOCK_MONOTONIC, &setting.it_value)) goto done;
    setting.it_value = advance(setting.it_value, PERIOD_NS);
    if (timerfd_settime(fd, TFD_TIMER_ABSTIME, &setting, NULL)) goto done;

    /* Deliberately do not consume the first four deadlines. */
    struct timespec target = advance(setting.it_value, 3 * PERIOD_NS);
    int err;
    do {
        err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &target, NULL);
    } while (err == EINTR);
    if (err) goto done;

    struct pollfd p = { .fd = fd, .events = POLLIN };
    if (poll(&p, 1, 2000) != 1 || p.revents != POLLIN) goto done;
    struct timespec before, after;
    uint64_t count;
    if (clock_gettime(CLOCK_MONOTONIC, &before) ||
        read(fd, &count, sizeof(count)) != (ssize_t)sizeof(count) ||
        clock_gettime(CLOCK_MONOTONIC, &after)) goto done;

    /* Sampling around read accommodates scheduler delay and boundary
     * crossings without confusing readiness with one expiration. */
    if (count < 4 || count < due(setting.it_value, before) ||
        count > due(setting.it_value, after)) goto done;

    const struct itimerspec off = {0};
    if (timerfd_settime(fd, 0, &off, NULL) || poll(&p, 1, 0) != 0)
        goto done;
    uint64_t stale;
    if (read(fd, &stale, sizeof(stale)) != -1 || errno != EAGAIN)
        goto done;
    struct itimerspec remaining;
    if (timerfd_gettime(fd, &remaining) ||
        remaining.it_value.tv_sec != 0 || remaining.it_value.tv_nsec != 0 ||
        remaining.it_interval.tv_sec != 0 || remaining.it_interval.tv_nsec != 0)
        goto done;
    rc = 0;
done:
    if (close(fd)) rc = 1;
    if (rc == 0)
        puts("timerfd_periodic_coalescing=PASS monotonic_count_bounds=PASS disarm=PASS cleanup=PASS");
    return rc;
}
