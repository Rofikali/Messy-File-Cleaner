# Filesystem security: scanner boundary

**Special-file acceptance:** [inode(7)](https://man7.org/linux/man-pages/man7/inode.7.html) distinguishes regular files, FIFOs and sockets. New independent scanner integration fixture contains a regular file, named pipe with no writer, and bound pathname AF_UNIX socket. Two scans must report only the regular file, preserve its content and special inode types, and balance file descriptors; cleanup removes only disposable fixtures. No production change. Device nodes, concurrent inode substitution and end-to-end worker/mover safety are outside this test. Exact-head CI required.

## Implemented scanner guarantees

The scanner now uses descriptor-relative traversal rather than building a path and calling stat() on it:

- Opens the requested root as a directory and rejects a symlink at the root path.
- Uses fstatat(..., AT_SYMLINK_NOFOLLOW) to inspect entries without following symlinks.
- Uses openat(..., O_DIRECTORY | O_NOFOLLOW) when descending into child directories.
- Visits regular files only. Symlinks, sockets, FIFOs, devices, and other special files are skipped.
- Builds event paths with checked dynamic allocation rather than a fixed 1024-byte buffer.
- Uses an explicit directory stack instead of recursive C calls.
- Returns a failure status when traversal cannot safely complete; batch mode reports that failure.

## Test coverage

The scanner integration test creates disposable temporary directories and verifies:

- A regular file is reported.
- A deeply nested file whose path exceeds the old 1024-byte limit is reported.
- A symlink to an outside file is not reported.
- A symlink to an outside directory is not traversed.
- A symlink used as the scan root is rejected.

The test fixtures are created only under /tmp in the CI runner.

## Important remaining limitation

A scanner event is still a pathname string. The file can be replaced after scanning but before a worker opens it. Therefore scanner no-follow checks alone do not eliminate the scanner-to-mover TOCTOU race.

The mover must independently open the source with no-follow semantics, verify the opened file with fstat, and use descriptor-relative destination operations. That work is a separate filesystem-mover milestone. Do not describe the whole application as fully symlink-race-safe until that milestone is complete.

The root's final path component is protected against symlink following. Canonical root policy and ancestor-component validation are also separate work; the current exact-string dangerous-root check is not a complete root authorization boundary.


## Linux selftest-style descriptor lifecycle reference

LINUX-SELFTEST-01 starts stage 5 of the upstream master plan after scoped CPU research. Existing injected/real exhaustion, descriptor count and scanner traversal features remain closed; this distinct check verifies actual live directory descriptor flags, not exhaustion. Independent Linux-only executable links real scanner without FC_SCANNER_TESTING; private mkdtemp root/child/file. During each of three callbacks inspect /proc/self/fd, compare fstat device/inode to both expected directory identities, require exactly one descriptor per identity with F_GETFD FD_CLOEXEC set; require zero matching descriptors before/after each complete scan, exact path and preserved one-byte file. Cleanup only owned fixture. This verifies close-on-exec flags, not a real exec inheritance experiment or process-wide descriptor leak absence. No special privileges, imports, dependencies, production changes, spending or release. Debug ASan/UBSan directly applied, Linux-only CTest timeout 15s. Local tests NOT RUN; exact-head CI pending; prior Ubuntu image job must be reconciled before merging. FS-01B/REL-01/license lock unchanged. Rollback test-only revert. Networking/business/API N/A.


### Actual exec inheritance — second candidate

Baseline a50d283385fea988edf91c8fe0f20e82021f8e92; #251 merged, 14/14 exact-head jobs PASS (CMake 45775502176, Ubuntu 45775502075, Docker 45775501998, Makefile 45775501973, FCJ4 TSan 45775501954). C17 Release 139758073039 and Debug 139758073052 each 69/69, scanner CLOEXEC test passed. #250 prior Ubuntu image workflow now PASS. Extend private fixture scanner callback with fork and execv /proc/self/exe; child re-stats both private directory identities, inventories its own descriptors, requires zero matches for ordinary exec and exactly one non-CLOEXEC root descriptor (zero child directory descriptors) for intentional positive control. The positive control is opened only in the child before exec, so it cannot affect parent inventory. Parent waitpid retries EINTR and rejects signal/nonzero exits; parent verifies both live CLOEXEC flags again before scan unwinds. Three scans yield three negative and three positive exec probes, exact callbacks, zero fixture directory descriptors after each scan and preserved file bytes. No shell, PATH search, external executable or actual user file. C17 Release CI emits verbose result; Debug direct ASan/UBSan retained. This is single-threaded test-only evidence, not process-wide inheritance or multithreaded fork safety proof. Local tests NOT RUN, exact-head CI pending; no production changes, imports, dependencies, spending or release; FS-01B/REL-01/license lock unchanged. Rollback test/CI revert.


### Abort and rescan after actual exec — third candidate

Baseline 482bdcdc6f028065cb1af949a2aaec842c0f7530; #252 merged, 13/13 returned exact-head jobs PASS (CMake 45775922037, Ubuntu 45775921981, Docker 45775922159, Makefile 45775921977; FCJ4 TSan not returned). C17 Release 139759428291 and Debug 139759428405 each 69/69; verbose Release confirms three negative and three positive real exec controls, three restorations and preserved bytes. Distinct descriptor lifecycle regression: after both child exec probes and live inode/flag recheck, return EACCES or EIO from callback, or request cooperative cancellation with a real atomic stop signal. Require scan -1 and exact errno captured before inventory, one callback, zero matching directory descriptors; reset state and require immediate successful scan of same private inodes with one callback and zero residual matching descriptors after each abort. Original three successful scans retained; total nine scan attempts, 18 child probes (nine negative/nine positive), two callback failures, one cancellation, three recoveries, preserved one-byte file. Printed live_directory_checks=18 counts two distinct directory identities per callback, not all repeated checks. Existing generic injected-error and cancellation features remain closed; this is specifically real exec plus inode-specific lifecycle verification, not new production behavior. Existing timeout and Debug direct ASan/UBSan retained. Local tests NOT RUN; exact-head CI pending. No multithreaded fork, arbitrary process leak, crash safety or source retirement proof. No source imports, new dependencies, spending or release; FS-01B/REL-01/license lock unchanged. Rollback test-only revert.


### Deep descriptor lifecycle across frame growth — fourth candidate

Baseline 589fb430d83af5cae716e97b23b579d804a77d58; #253 merged, 12 completed exact-head jobs PASS (CMake 45776718830, Makefile 45776718965, Docker 45776718845, Ubuntu 45776718841 three standard jobs PASS, image job still running; FCJ4 TSan not returned). C17 Release 139762019066 and Debug 139762019027 each 69/69; verbose Release confirmed 18 actual exec probes (9 negative/9 positive), 9 restorations, two callback errors, one cancellation, three recoveries and preserved bytes. New independent private 20-directory chain crosses scanner's initial 16-frame array capacity. Generalize inode inventory and self-exec arguments to all 20 distinct directory identities; three deep scan attempts (success, cancellation, successful rescan), 60 live directory identities across callbacks, six actual exec controls (3+3), zero fixture directory descriptors after every scan and preserved one-byte file. Existing shallow cases retained. Child checks all fixture inodes, intentional positive root-only control; no shell or external program. Checks are identity-specific, not whole-process descriptor counts or proof of all allocation boundaries. Existing Debug direct ASan/UBSan and CTest timeout retained. Prior Ubuntu image completion must be reconciled before merge. Local tests NOT RUN; exact-head CI pending. No production changes, imports, dependencies, spending or release; FS-01B/REL-01/license lock unchanged. Rollback test-only revert.
