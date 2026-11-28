#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef SYS_pidfd_open
int main(void)
{
    puts("NOT VERIFIED: installed headers lack pidfd_open");
    return 77;
}
#else
int main(void)
{
    /* Only the current process is targeted; no signals or other processes. */
    const pid_t self = getpid();
    int fd = (int)syscall(SYS_pidfd_open, self, 0U);
    if (fd < 0) {
        if (errno == ENOSYS || errno == EPERM || errno == EACCES) {
            puts("NOT VERIFIED: pidfd_open unavailable or restricted");
            return 77;
        }
        perror("pidfd_open self");
        return 1;
    }

    int invalid = (int)syscall(SYS_pidfd_open, self, UINT_MAX);
    int saved = errno;
    if (invalid >= 0) {
        close(invalid);
        close(fd);
        return 1;
    }
    if (saved != EINVAL) {
        close(fd);
        return 1;
    }

    struct pollfd p = { .fd = fd, .events = POLLIN, .revents = 0 };
    if (poll(&p, 1, 0) != 0) {
        close(fd);
        return 1;
    }
    if (close(fd)) return 1;

    /* Single-threaded reference: no descriptor allocation between close
       and the two negative controls. */
    if (fcntl(fd, F_GETFD) != -1 || errno != EBADF) return 1;
    if (poll(&p, 1, 0) != 1 || p.revents != POLLNVAL) return 1;
    puts("pidfd_invalid_flags=PASS live_self=PASS closed_ebadf=PASS closed_pollnval=PASS");
    return 0;
}
#endif
