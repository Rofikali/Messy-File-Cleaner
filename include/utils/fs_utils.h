// ./include/utils/fs_utils.h
#ifndef FS_UTILS_H
#define FS_UTILS_H

int ensure_dir(const char* path);
int file_exists(const char* path);
int copy_file_safe(const char* src, const char* dest);

#endif