#include "window.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void registry_global(void *data, struct wl_registry *registry,
                           uint32_t name, const char *interface,
                           uint32_t version) {
    (void) version;
    Window* state = data;
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        state->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        state->layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        if (!state->output) {
            state->output = wl_registry_bind(registry, name, &wl_output_interface, 1);
        }
    }
}

static void layer_surface_configure(void *data,
                                   struct zwlr_layer_surface_v1 *surface,
                                   uint32_t serial, uint32_t w, uint32_t h) {
    (void)data;
    (void)w;
    (void)h;
    zwlr_layer_surface_v1_ack_configure(surface, serial);
}

static void layer_surface_closed(void *data,
                                struct zwlr_layer_surface_v1 *surface) {
    (void)surface;
    Window* state = data;
    state->active = 0;
}

Window* window_init(int width, int height) {
    Window* wl;

    wl = malloc(sizeof(Window));
    wl->registry_listener = malloc(sizeof(struct wl_registry_listener));
    wl->layer_surface_listener = malloc(sizeof(struct zwlr_layer_surface_v1_listener));
    wl->compositor = NULL;
    wl->shm = NULL;
    wl->layer_shell = NULL;
    wl->output = NULL;
    wl->width = width;
    wl->height= height;
    wl->active = 1;


    wl->display = wl_display_connect(NULL);
    if (!wl->display) {
        perror("Failed to connect to display");
        return NULL;
    }

    wl->registry = wl_display_get_registry(wl->display);
    wl->registry_listener->global = registry_global;
    wl->registry_listener->global_remove = NULL;
    wl_registry_add_listener(wl->registry, wl->registry_listener, wl);
    wl_display_roundtrip(wl->display);

    if (!wl->compositor || !wl->shm || !wl->layer_shell) {
        perror("Failed to load wayland interface");
        return NULL;
    }

    wl->surface = wl_compositor_create_surface(wl->compositor);
    wl->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        wl->layer_shell, wl->surface, wl->output, ZWLR_LAYER_SHELL_V1_LAYER_TOP, "panel");

    zwlr_layer_surface_v1_set_size(wl->layer_surface, width, height);
    zwlr_layer_surface_v1_set_anchor(wl->layer_surface,
        ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    zwlr_layer_surface_v1_set_exclusive_zone(wl->layer_surface, width);

    wl->layer_surface_listener->configure = layer_surface_configure;
    wl->layer_surface_listener->closed = layer_surface_closed;
    zwlr_layer_surface_v1_add_listener(wl->layer_surface, wl->layer_surface_listener, wl);

    wl_surface_commit(wl->surface);
    wl_display_roundtrip(wl->display);

    return wl;
}

void window_draw_buffer(Window* win, struct wl_buffer* buf) {
    wl_surface_attach(win->surface, buf, 0, 0);
    wl_surface_damage(win->surface, 0, 0, win->width, win->height);
    wl_surface_commit(win->surface);
}

void window_handle_events(Window* win) {
    wl_display_dispatch_pending(win->display);
    wl_display_flush(win->display);
}

void window_destroy(Window* win) {
    zwlr_layer_surface_v1_destroy(win->layer_surface);
    wl_surface_destroy(win->surface);
    wl_display_disconnect(win->display);
    free(win->registry_listener);
    free(win);
}
