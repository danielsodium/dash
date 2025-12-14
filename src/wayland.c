#include "wayland.h"

#include <string.h>
#include <stdlib.h>

static Wayland* w;

void registry_global(void *data, struct wl_registry *registry,
                           uint32_t name, const char *interface,
                           uint32_t version) {
    (void) data; (void) version;
    if (strcmp(interface, wl_compositor_interface.name) == 0)
        w->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    else if (strcmp(interface, wl_shm_interface.name) == 0)
        w->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0)
        w->layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
    else if (strcmp(interface, wl_output_interface.name) == 0 && !w->output)
        w->output = wl_registry_bind(registry, name, &wl_output_interface, 1);
    else if (strcmp(interface, wl_seat_interface.name) == 0)
        w->seat = wl_registry_bind(registry, name, &wl_seat_interface, 5);
}

static struct wl_registry_listener _registry_listener = { 
    .global_remove = NULL, .global = registry_global
};

static void _layer_surface_configure(void *data,
                                   struct zwlr_layer_surface_v1 *surface,
                                   uint32_t serial, uint32_t wa, uint32_t h) {
    (void)data; (void)wa; (void)h;
    zwlr_layer_surface_v1_ack_configure(surface, serial);
    w->surfaces_loaded++;
}

static void _layer_surface_closed(void *data,
                                struct zwlr_layer_surface_v1 *surface) {
    (void)data;
    zwlr_layer_surface_v1_destroy(surface);
}

struct wl_surface* wayland_create_surface(size_t width, size_t height, int anchor, int layer) {
    struct wl_surface* s = wl_compositor_create_surface(w->compositor);
    struct zwlr_layer_surface_v1* ls = zwlr_layer_shell_v1_get_layer_surface(
        w->layer_shell, s, NULL, layer, "widget");
    static const struct zwlr_layer_surface_v1_listener ls_listener = (struct zwlr_layer_surface_v1_listener) {
        .configure = _layer_surface_configure, .closed = _layer_surface_closed
    };

    zwlr_layer_surface_v1_set_size(ls, width, height);
    zwlr_layer_surface_v1_set_anchor(ls, anchor);

    // TOP/BOTTOM are exclusive layers
    if (layer != ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY &&
        layer != ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND) {
        int vert = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
        if ((anchor ^ vert) == 0) {
            zwlr_layer_surface_v1_set_exclusive_zone(ls, width);
        } else {
            zwlr_layer_surface_v1_set_exclusive_zone(ls, height);
        }
    } else {
        zwlr_layer_surface_v1_set_exclusive_zone(ls, -1);
    }

    zwlr_layer_surface_v1_set_keyboard_interactivity(ls,
        layer == ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY ? 
            ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND :
            ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE
        );
    zwlr_layer_surface_v1_add_listener(ls, &ls_listener, w);

        // Add surface to overlord
    if (w->surfaces_size >= w->surfaces_capacity) {
        w->surfaces_capacity *= 2;
        w->surfaces = realloc(w->surfaces, w->surfaces_capacity * sizeof(struct wl_surface*));
        w->layer_surfaces = realloc(w->layer_surfaces, w->surfaces_capacity * sizeof(struct wl_layer_surfaces*));
    }

    w->surfaces[w->surfaces_size] = s;
    w->layer_surfaces[w->surfaces_size] = ls;
    w->surfaces_size++;

    return s;
}

int wayland_init() {
    w = malloc(sizeof(Wayland));

    *w = (Wayland) {
        .surfaces_capacity = 1,
        .surfaces = malloc(sizeof(struct wl_surface*)),
        .layer_surfaces = malloc(sizeof(struct zwlr_layer_surface_v1*))
    };
    w->display = wl_display_connect(NULL);
    if (!w->display) {
        perror("Failed to connect to display");
        return 1;
    }

    // Setup registry
    w->registry = wl_display_get_registry(w->display);
    wl_registry_add_listener(w->registry, &_registry_listener, w);
 
    wl_display_roundtrip(w->display);
    if (!w->compositor || !w->shm || !w->layer_shell) {
        perror("Failed to load wayland interface");
        return 1;
    }

    return 0;
}

int wayland_display_fd() {
    return wl_display_get_fd(w->display);
}

void wayland_commit_surfaces() {
    for (int i = 0; i < w->surfaces_size; i++) {
        wl_surface_commit(w->surfaces[i]);
    }
    wl_display_roundtrip(w->display);
}
void wayland_prepare_display() {
    wl_display_flush(w->display);
    while (wl_display_prepare_read(w->display) != 0) {
        wl_display_dispatch_pending(w->display);
    }
}
int wayland_dispatch() {
    return wl_display_dispatch(w->display);
}

void wayland_display_events() {
    wl_display_read_events(w->display);
    wl_display_dispatch_pending(w->display);
}
void wayland_flush() {
    wl_display_flush(w->display);
}
void wayland_cancel_read() {
    wl_display_cancel_read(w->display);
}

struct wl_seat* wayland_seat() {
    return w->seat;
}
struct wl_shm* wayland_shm() {
    return w->shm;
}

void wayland_roundtrip() {
    wl_display_roundtrip(w->display);
}
void wayland_hide_surface(struct wl_surface* surface) {
    wl_surface_attach(surface, NULL, 0, 0);
    wl_surface_commit(surface);
}
void wayland_commit(struct wl_surface* surface) {
    wl_surface_commit(surface);
}

int wayland_surfaces_loaded() {
    return w->surfaces_loaded;
}

