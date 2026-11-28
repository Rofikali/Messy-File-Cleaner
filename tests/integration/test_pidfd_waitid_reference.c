#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#if !defined(SYS_pidfd_open) || !defined(FC_HAVE_P_PIDFD)
int main(void)
{
    puts("NOT VERIFIED: pidfd_open or P_PIDFD missing from headers");
    return 77;
}
#else
static int expected_exit(const siginfo_t *info, pid_t child)
{
    return info->si_pid == child && info->si_signo == SIGCHLD &&
           info->si_code == CLD_EXITED && info->si_status == 41;
}

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
        _exit(n == 0 ? 41 : 42);
    }
    close(gate[0]);
    int rc = 1, reaped = 0, status = 0;
    int fd = (int)syscall(SYS_pidfd_open, child, 0U);
    if (fd < 0) {
        if (errno == ENOSYS || errno == EPERM || errno == EACCES) rc = 77;
        goto done;
    }
    siginfo_t info = {0};
    if (waitid(P_PIDFD, (id_t)fd, &info, WEXITED | WNOHANG | WNOWAIT)) {
        if (errno == EINVAL || errno == ENOSYS || errno == EPERM || errno == EACCES)
            rc = 77;
        goto done;
    }
    if (info.si_pid != 0) goto done;
    if (close(gate[1])) {
        gate[1] = -1;
        goto done;
    }
    gate[1] = -1;
    struct pollfd p = { .fd = fd, .events = POLLIN, .revents = 0 };
    if (poll(&p, 1, 2000) != 1 || !(p.revents & POLLIN) ||
        (p.revents & (POLLERR | POLLNVAL))) goto done;
    for (int i = 0; i < 2; ++i) {
        info = (siginfo_t){0};
        if (waitid(P_PIDFD, (id_t)fd, &info, WEXITED | WNOHANG | WNOWAIT) ||
            !expected_exit(&info, child)) goto done;
    }
    info = (siginfo_t){0};
    if (waitid(P_PIDFD, (id_t)fd, &info, WEXITED | WNOHANG)) goto done;
    if (info.si_pid != 0) reaped = 1;
    if (!expected_exit(&info, child)) goto done;
    info = (siginfo_t){0};
    if (waitid(P_PIDFD, (id_t)fd, &info, WEXITED | WNOHANG) != -1 ||
        errno != ECHILD) goto done;
    rc = 0;
done:
    if (gate[1] >= 0 && close(gate[1])) rc = 1;
    if (!reaped) {
        pid_t got;
        do {
            got = waitpid(child, &status, 0);
        } while (got < 0 && errno == EINTR);
        if (got != child || !WIFEXITED(status) || WEXITSTATUS(status) != 41)
            rc = 1;
    }
    if (fd >= 0 && close(fd)) rc = 1;
    if (rc == 0) puts("waitid_live_empty=PASS peek_twice=PASS exact_exit=PASS reaped_once=PASS echild=PASS");
    if (rc == 77) puts("NOT VERIFIED: pidfd or waitid P_PIDFD unavailable or restricted");
    return rc;
}
#endif
