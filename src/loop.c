#include "loop.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <errno.h>

Loop* loop_create() {
    Loop* l = malloc(sizeof(Loop));
    l->fds_size = 0;
    l->fds_cap = 2;
    l->fds = malloc(2 * sizeof(int));
    l->args = malloc(2 * sizeof(void*));
    l->callbacks = malloc(2 * sizeof(LoopCallback));
    l->epoll = epoll_create1(0);
    if (l->epoll == -1) {
        perror("Failed to create epoll");
        return NULL;
    }
    return l;
}

void loop_add_fd(Loop* l, int fd, LoopCallback cb, void* args) {
    struct epoll_event ev = {0};
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(l->epoll, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("Failed to epoll");
        return;
    }

    if (l->fds_size >= l->fds_cap) {
        l->fds_cap *= 2;
        l->fds = realloc(l->fds, sizeof(int) * l->fds_cap);
        l->callbacks = realloc(l->callbacks , sizeof(LoopCallback) * l->fds_cap);
        l->args = realloc(l->args, sizeof(void*) * l->fds_cap);
    }
    l->callbacks[l->fds_size] = cb;
    l->fds[l->fds_size] = fd;
    l->args[l->fds_size] = args;
    l->fds_size++;
}

void loop_add_timer(Loop* l, long interval, LoopCallback cb) {
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (fd == -1) {
        perror("Failed to create timer");
        return;
    }

    struct itimerspec spec = {0};
    spec.it_value.tv_sec = interval / 1000;
    spec.it_value.tv_nsec = (interval % 1000) * 1000000;
    spec.it_interval = spec.it_value;
    
    if (timerfd_settime(fd, 0, &spec, NULL) == -1) {
        close(fd);
        return;
    }

    loop_add_fd(l, fd, cb, NULL);
}

void loop_run(Loop* l) {
    int nfds = epoll_wait(l->epoll, l->events, 10, -1);
    if (nfds == -1) {
        if (errno == EINTR) return;
        perror("Failed to wait epoll");
        return;
    }
    for (int i = 0; i < nfds; i++) {
        for (int j = 0; j < l->fds_size; j++) {
            int fd = l->events[i].data.fd;
            if (fd == l->fds[j]) {
                l->callbacks[j](fd, l->args[j]);
            }
        }
    }

}
