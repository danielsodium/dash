#ifndef _WAYLAND_H_
#define _WAYLAND_H_

#include <stdint.h>
#include <stddef.h>

#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include <xkbcommon/xkbcommon.h>

#include "common.h"

struct Wayland {
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
    int surfaces_loaded;
};

int wayland_init();
int wayland_display_fd();
void wayland_commit_surfaces();
void wayland_prepare_display();
void wayland_hide_surface(struct wl_surface* surface);
void wayland_commit(struct wl_surface* surface);
void wayland_display_events();
void wayland_flush();
void wayland_cancel_read();
void wayland_roundtrip();
int wayland_surfaces_loaded();
int wayland_dispatch();
struct wl_surface* wayland_create_surface(size_t width, size_t height, int anchor, int layer);

struct wl_seat* wayland_seat();
struct wl_shm* wayland_shm();


#endif
