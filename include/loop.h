#ifndef _LOOP_H_
#define _LOOP_H_

#include <sys/epoll.h>
#include <sys/socket.h>
#include <time.h>

typedef void (*LoopCallback)(int);

typedef struct Loop {
    int epoll;
    struct epoll_event events[32];

    int* fds;
    LoopCallback* callbacks;
    int fds_size;
    int fds_cap;
} Loop;


Loop* loop_create();
void loop_add_fd(Loop* l, int fd, LoopCallback cb);
void loop_add_timer(Loop* l, long interval, LoopCallback cb);
void loop_run(Loop* l);

#endif
