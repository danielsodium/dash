#ifndef _OVERLORD_H_
#define _OVERLORD_H_

#include <stdio.h>
#include <stdlib.h>

#include <cairo/cairo.h>
#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include <xkbcommon/xkbcommon.h>

#include "canvas.h"
#include "common.h"

struct Overlord {
    int active;

    // idk why its 32
    struct epoll_event events[32];
    int epoll_fd;
    int display_fd;
    int step_fd;
    int repeat_fd;
    long step_interval;

    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_registry_listener registry_listener;

    struct wl_compositor* compositor;
    struct wl_shm* shm;
    struct zwlr_layer_shell_v1* layer_shell;
    struct wl_output* output;
    struct wl_seat* seat;

    Keyboard* keyboard;
};

// Scan to see if any other overlords are active
// return true if found
int overlord_scan();
int overlord_run();

#endif
