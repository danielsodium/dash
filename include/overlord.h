#ifndef _OVERLORD_H_
#define _OVERLORD_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include <cairo/cairo.h>
#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include <xkbcommon/xkbcommon.h>

#include "canvas.h"
#include "loop.h"
#include "common.h"

struct Overlord {
    int active;
    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_compositor* compositor;
    struct wl_shm* shm;
    struct zwlr_layer_shell_v1* layer_shell;
    struct wl_output* output;
    struct wl_seat* seat;

    struct wl_surface** surfaces;
    struct zwlr_layer_surface_v1** layer_surfaces;
    int surfaces_size;
    int surfaces_capacity;

    Keyboard* keyboard;
    Widget* widget;
    Loop* loop;
};

int overlord_run(int sock);

#endif
