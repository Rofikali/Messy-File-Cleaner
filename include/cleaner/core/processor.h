
// cleaner/include\processor.h
#ifndef CLEANER_PROCESSOR_H
#define CLEANER_PROCESSOR_H

#ifdef __cplusplus
extern "C"
{
#endif

    /* =========================================
       Time Mode
       ========================================= */

    typedef enum
    {
        TIME_MODE_MODIFIED = 0,
        TIME_MODE_CREATION = 1
    } TimeMode;

    /* =========================================
       Public API
       ========================================= */

    void processor_set_mode(TimeMode mode);
    void processor_set_target(const char *target);
    void processor_set_dry_run(int dry);

    void processor_process_file(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* CLEANER_PROCESSOR_H */