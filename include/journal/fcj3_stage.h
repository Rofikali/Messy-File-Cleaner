#ifndef FCJ3_STAGE_H
#define FCJ3_STAGE_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * Test-only anonymous FCJ3 staging lifecycle.
 *
 * The destination directory descriptor is borrowed and must remain open.
 * This object owns its staging and procfs descriptors. Do not copy an
 * initialized object. Closing never unlinks a staging or published entry.
 */
typedef struct
{
    int directory_fd;
    int staging_fd;
    int procfs_fd;
    dev_t directory_device;
    ino_t directory_inode;
    dev_t staging_device;
    ino_t staging_inode;
    uid_t expected_owner;
    char staging_component[256];
    int initialized;
    int copied;
    int poisoned;
    int synced;
    int linked;
    int published;
} Fcj3Stage;

/* Create an anonymous O_TMPFILE inode in the already-open destination
 * directory. The component must be a single .fc-tmp-* name.
 */
int fcj3_stage_open_at(Fcj3Stage *stage, int directory_fd,
                       const char *staging_component, uid_t expected_owner);

/* Borrow the anonymous staging descriptor before sync; do not retain it or write after sync. */
int fcj3_stage_fd(const Fcj3Stage *stage);

/* Copy an already-open regular source descriptor into the anonymous stage.
 * The caller must open it safely (normally O_RDONLY | O_NOFOLLOW). The helper
 * checks source identity, size, owner, mode, mtime and ctime before/after.
 * Any copy uncertainty poisons the stage.
 */
int fcj3_stage_copy_from(Fcj3Stage *stage, int source_fd,
                         struct stat *source_snapshot, uint64_t *bytes_copied);

/* fsync the anonymous file and return its validated stat snapshot. */
int fcj3_stage_sync(Fcj3Stage *stage, struct stat *staging_metadata);

/* Link only through verified /proc/self/fd + AT_SYMLINK_FOLLOW, verify exact
 * inode identity, then fsync the destination directory.
 */
int fcj3_stage_link(Fcj3Stage *stage);

/* Publish with renameat2(RENAME_NOREPLACE), verify the final inode identity,
 * then fsync the destination directory. EEXIST never overwrites.
 */
int fcj3_stage_publish(Fcj3Stage *stage, const char *final_component);

/* Close owned descriptors. Never removes a visible staging/final entry. */
int fcj3_stage_close(Fcj3Stage *stage);

#ifdef FC_TESTING
/* Test-only deterministic partial-pwrite failure injection. */
void fcj3_stage_test_fail_write_after(size_t after_bytes, int error_code);
void fcj3_stage_test_grow_source_after_copy(int writable_source_fd);
#endif

#endif
