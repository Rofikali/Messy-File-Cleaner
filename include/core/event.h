#ifndef EVENT_H
#define EVENT_H

typedef enum
{
    FILE_CREATED,
    FILE_FOUND
} EventType;

typedef struct
{
    char path[1024];
    EventType type;
} FileEvent;

#endif