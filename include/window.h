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
#include "common.h"

struct Window {

    int active;
    int width;
    int height;

    int draw_attached;
    int init_attached;
    int keyboard_attached;
    int step_attached;
    int destroy_attached;

    long step_interval;

    int epoll_fd;
    int display_fd;
    int step_fd;

    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_registry_listener* registry_listener;

    struct wl_compositor* compositor;
    struct wl_shm* shm;
    struct zwlr_layer_shell_v1* layer_shell;
    struct wl_output* output;
    struct wl_seat* seat;

    struct wl_surface* surface;
    struct zwlr_layer_surface_v1 *layer_surface;
    struct zwlr_layer_surface_v1_listener* layer_surface_listener;

    Canvas* canvas;
    Keyboard* keyboard;

    void (*init)(cairo_t*, void*);
    void (*draw)(cairo_t*, int*, void*);
    void (*destroy)(void*);

    int (*step)(int*, void*);
    int (*on_key)(xkb_keysym_t*, int*, void*);

    void* data;

};

Window* window_create(int width, int height, int anchor, int layer);

void window_attach_init(Window* w, void (*init)(cairo_t*, void*));
void window_attach_data(Window *w, void* data);
void window_attach_destroy(Window* w, void(*destroy)(void*));

void window_attach_draw(Window* w, void(*draw_func)(cairo_t*, int*, void*));
void window_attach_step(Window* w, int interval, int(*step_func)(int*, void*));
void window_attach_keyboard_listener(Window* w, int(*on_key_func)(xkb_keysym_t*, int*, void*));

void window_run(Window* w);

#endif
