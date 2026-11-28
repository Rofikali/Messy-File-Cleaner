#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/epoll.h>
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
static int unavailable(int error)
{
    return error == ENOSYS || error == EPERM || error == EACCES;
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
        _exit(n == 0 ? 31 : 32);
    }
    close(gate[0]);
    int rc = 1, status = 0, self_fd = -1, ep = -1;
    int child_fd = (int)syscall(SYS_pidfd_open, child, 0U);
    if (child_fd < 0) {
        if (unavailable(errno)) rc = 77;
        goto done;
    }
    self_fd = (int)syscall(SYS_pidfd_open, getpid(), 0U);
    if (self_fd < 0) goto done;
    ep = epoll_create1(EPOLL_CLOEXEC);
    if (ep < 0) goto done;
    struct epoll_event child_event = { .events = EPOLLIN, .data.u64 = 1 };
    struct epoll_event self_event = { .events = EPOLLIN, .data.u64 = 2 };
    struct epoll_event events[2];
    if (epoll_ctl(ep, EPOLL_CTL_ADD, child_fd, &child_event) ||
        epoll_ctl(ep, EPOLL_CTL_ADD, self_fd, &self_event)) goto done;
    if (epoll_wait(ep, events, 2, 0) != 0) goto done;
    if (close(gate[1])) {
        gate[1] = -1;
        goto done;
    }
    gate[1] = -1;
    if (epoll_wait(ep, events, 2, 2000) != 1 ||
        events[0].data.u64 != 1 || !(events[0].events & EPOLLIN) ||
        (events[0].events & EPOLLERR)) goto done;
    if (epoll_ctl(ep, EPOLL_CTL_DEL, child_fd, NULL) ||
        epoll_wait(ep, events, 2, 0) != 0) goto done;
    rc = 0;
done:
    if (gate[1] >= 0 && close(gate[1])) rc = 1;
    pid_t reaped;
    do {
        reaped = waitpid(child, &status, 0);
    } while (reaped < 0 && errno == EINTR);
    if (reaped != child || !WIFEXITED(status) || WEXITSTATUS(status) != 31)
        rc = 1;
    if (ep >= 0 && close(ep)) rc = 1;
    if (self_fd >= 0 && close(self_fd)) rc = 1;
    if (child_fd >= 0 && close(child_fd)) rc = 1;
    if (rc == 0) puts("epoll_initial_empty=PASS child_tag=PASS live_self_quiet=PASS deregister=PASS reaped=PASS");
    if (rc == 77) puts("NOT VERIFIED: pidfd_open unavailable or restricted");
    return rc;
}
#endif
