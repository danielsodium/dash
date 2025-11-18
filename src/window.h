#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <stdio.h>
#include <stdlib.h>

#include <cairo/cairo.h>
#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include <xkbcommon/xkbcommon.h>
#include "canvas.h"

typedef struct {

    int active;
    int width;
    int height;

    int keyboard_attached;
    int step_attached;

    long step_interval;

    int epoll_fd;
    int display_fd;
    int step_fd;
    int draw_fd;

    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_registry_listener* registry_listener;

    struct wl_compositor* compositor;
    struct wl_shm* shm;
    struct zwlr_layer_shell_v1* layer_shell;
    struct wl_output* output;
    struct wl_seat* seat;

    struct wl_seat_listener* seat_listener;
    struct wl_keyboard* keyboard;
    struct wl_keyboard_listener* keyboard_listener;
    struct xkb_keymap* keyboard_keymap;
    struct xkb_state* keyboard_state;
    struct xkb_context* keyboard_context;

    struct wl_surface* surface;
    struct zwlr_layer_surface_v1 *layer_surface;
    struct zwlr_layer_surface_v1_listener* layer_surface_listener;

    Canvas* canvas;
    int (*draw)(cairo_t*, int*, void*);
    void (*step)(int*, void*);
    void (*on_key)(xkb_keysym_t*, int*, void*);

    void* data;

} Window;

Window* window_create(int width, int height, int anchor, int layer);
void window_commit(Window* w, int(*draw_func)(cairo_t*, int*, void*));
void window_draw(Window* w);
void window_handle_events(Window* w);

void window_attach_keyboard_listener(Window* w, void(*on_key_func)(xkb_keysym_t*, int*, void*));
void window_attach_step(Window* w, int interval, void(*step_func)(int*, void*));

void window_loop(Window* w);
void window_destroy(Window* w);

void* window_get_data(Window* w);
void window_set_data(Window* w, void* data);

#endif
