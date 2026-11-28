#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/wait.h>
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
    int gate[2];
    if (pipe2(gate, O_CLOEXEC)) return 1;
    pid_t child = fork();
    if (child < 0) {
        close(gate[0]);
        close(gate[1]);
        return 1;
    }
    if (child == 0) {
        char byte;
        close(gate[1]);
        ssize_t n;
        do {
            n = read(gate[0], &byte, 1);
        } while (n < 0 && errno == EINTR);
        close(gate[0]);
        _exit(n == 0 ? 23 : 24);
    }
    close(gate[0]);
    int rc = 1, status = 0;
    int fd = (int)syscall(SYS_pidfd_open, child, 0U);
    if (fd < 0) {
        if (errno == ENOSYS || errno == EPERM || errno == EACCES) {
            puts("NOT VERIFIED: pidfd_open unavailable or restricted");
            rc = 77;
        }
        goto done;
    }
    int flags = fcntl(fd, F_GETFD);
    struct pollfd p = { .fd = fd, .events = POLLIN, .revents = 0 };
    if (flags < 0 || !(flags & FD_CLOEXEC) || poll(&p, 1, 0) != 0)
        goto done;
    if (close(gate[1])) {
        gate[1] = -1;
        goto done;
    }
    gate[1] = -1;
    if (poll(&p, 1, 2000) != 1 || !(p.revents & POLLIN) ||
        (p.revents & (POLLERR | POLLNVAL))) goto done;
    rc = 0;
done:
    if (gate[1] >= 0 && close(gate[1])) rc = 1;
    pid_t reaped;
    do {
        reaped = waitpid(child, &status, 0);
    } while (reaped < 0 && errno == EINTR);
    if (reaped != child || !WIFEXITED(status) || WEXITSTATUS(status) != 23)
        rc = 1;
    if (fd >= 0 && close(fd)) rc = 1;
    if (rc == 0) puts("pidfd_cloexec=PASS live_not_ready=PASS exit_ready=PASS reaped=PASS");
    return rc;
}
#endif
