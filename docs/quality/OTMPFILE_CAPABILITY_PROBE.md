# Anonymous staging capability probe

## Purpose

The `otmpfile_capability` CTest checks whether the current Linux runtime and
filesystem support the exact primitive proposed for journal-owned staging:

1. Create an anonymous inode with `openat(..., O_TMPFILE | O_RDWR)`.
2. Write and fsync the inode.
3. Publish a directory entry with `linkat(fd, "", dirfd, name, AT_EMPTY_PATH)`.
4. Verify the linked entry is a regular file with the same device and inode.
5. Unlink the probe entry and fsync the directory.

The test uses a unique disposable directory below `/tmp` and removes only the
single entry it created. It does not inspect or modify project/user files.

## Result interpretation

- **PASS**: all tested operations succeeded and inode identity matched.
- **SKIP (CTest return code 77)**: the runtime/filesystem does not support
  `O_TMPFILE` or `AT_EMPTY_PATH` linking under the current permissions.
- **FAIL**: an unexpected I/O, cleanup, or identity-verification error occurred.

A skip is not proof that every filesystem is unsupported; it describes only
the environment where the test ran. Run this probe on each filesystem in the
declared support matrix.

## Security boundary

This is a capability probe only. It does not change production mover behavior,
journal format, recovery behavior, or temporary-file cleanup. It does not prove
that recovery cleanup is race-free in a directory writable by an untrusted
principal. That threat model still requires a reviewed design and adversarial
tests.

## Build and run

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --target test_otmpfile_capability --parallel 2
ctest --test-dir build -R '^otmpfile_capability$' --output-on-failure
```
