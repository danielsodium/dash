#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <stdio.h>
#include <stdlib.h>

#include <cairo/cairo.h>
#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"

typedef struct {

    int active;
    int height;
    int width;

    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_registry_listener* registry_listener;

    struct wl_compositor* compositor;
    struct wl_shm* shm;
    struct zwlr_layer_shell_v1* layer_shell;
    struct wl_output* output;

    struct wl_surface* surface;
    struct zwlr_layer_surface_v1_listener* layer_surface_listener;
    struct zwlr_layer_surface_v1 *layer_surface;

} Window;

Window* window_init(int width, int height);

void window_draw_buffer(Window* win, struct wl_buffer* buf);
void window_handle_events(Window* win);
void window_destroy(Window* win);

#endif
