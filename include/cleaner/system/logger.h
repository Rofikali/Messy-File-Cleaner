
// cleaner/include\logger.h

#ifndef LOGGER_H
#define LOGGER_H

typedef enum
{
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevel;

void logger_init(const char *log_dir);
void logger_log(LogLevel level,
                const char *event,
                const char *file_path);
void logger_close();

#endif