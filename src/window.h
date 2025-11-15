#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <stdio.h>
#include <stdlib.h>

#include <cairo/cairo.h>
#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include "draw.h"

typedef struct {

    int active;

    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_registry_listener* registry_listener;

    struct wl_compositor* compositor;
    struct wl_shm* shm;
    struct zwlr_layer_shell_v1* layer_shell;
    struct wl_output* output;
#ifdef DRUN
    struct wl_seat* seat;
    struct wl_listener* seat_listener;
#endif

    struct wl_surface* surface;
    struct zwlr_layer_surface_v1 *layer_surface;
    struct zwlr_layer_surface_v1_listener* layer_surface_listener;

    Canvas* canvas;

} Window;

Window* window_init();

void window_draw(Window* win);
void window_handle_events(Window* win);
void window_destroy(Window* win);

#endif
