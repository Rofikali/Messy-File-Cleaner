#define _GNU_SOURCE
#include <dirent.h>
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
/* Standalone single-threaded test: the directory's own descriptor
   is included in each count and closed before returning. */
static int descriptor_count(void)
{
    DIR *dir = opendir("/proc/self/fd");
    if (!dir) return -1;
    int count = 0;
    int error = 0;
    for (;;) {
        errno = 0;
        struct dirent *entry = readdir(dir);
        if (!entry) {
            error = errno;
            break;
        }
        if (entry->d_name[0] != '.') ++count;
    }
    if (closedir(dir) || error) return -1;
    return count;
}

static int one_cycle(int allow_skip, int force_unwind)
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
        _exit(n == 0 ? 51 : 52);
    }
    close(gate[0]);
    int rc = 1, status = 0;
    int fd = (int)syscall(SYS_pidfd_open, child, 0U);
    if (fd < 0) {
        if (allow_skip && (errno == ENOSYS || errno == EPERM ||
                           errno == EACCES)) rc = 77;
        goto done;
    }
    /* Deliberately abandon monitoring after acquisition to exercise unwind. */
    if (force_unwind) {
        rc = 0;
        goto done;
    }
    struct pollfd p = { .fd = fd, .events = POLLIN, .revents = 0 };
    if (poll(&p, 1, 0) != 0) goto done;
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
    pid_t got;
    do {
        got = waitpid(child, &status, 0);
    } while (got < 0 && errno == EINTR);
    if (got != child || !WIFEXITED(status) || WEXITSTATUS(status) != 51)
        rc = 1;
    if (fd >= 0 && close(fd)) rc = 1;
    return rc;
}

int main(void)
{
    int baseline = descriptor_count();
    if (baseline < 0) {
        puts("NOT VERIFIED: /proc/self/fd enumeration unavailable");
        return 77;
    }
    for (int i = 0; i < 32; ++i) {
        int rc = one_cycle(i == 0, i % 2 != 0);
        if (descriptor_count() != baseline) return 1;
        int status = 0;
        if (waitpid(-1, &status, WNOHANG) != -1 || errno != ECHILD)
            return 1;
        if (rc == 77) {
            puts("NOT VERIFIED: pidfd_open unavailable or restricted");
            return 77;
        }
        if (rc != 0) return 1;
    }
    printf("pidfd_cycles=32 normal=16 unwind=16 fd_balance=PASS no_children=PASS baseline=%d\n", baseline);
    return 0;
}
#endif
