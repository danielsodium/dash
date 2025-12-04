#include "overlord.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/timerfd.h>

#include "widget.h"
#include "drun.h"
#include "keyboard.h"

// SINGLETON OVERLORD
static Overlord* o = NULL;

// Callback functions
static void _registry_global(void *data, struct wl_registry *registry, 
                             uint32_t name, const char *interface, uint32_t version);
static int _init(int sock);
static int _destroy();

int overlord_run(int sock) {
    o = calloc(1, sizeof(Overlord));
    if (!o) {
        perror("Failed to allocate the overlord");
        return 1;
    }

    if (_init(sock)) {
        free(o);
        return 1;
    }

    // LOOP
    for (int i = 0; i < o->surfaces_size; i++) {
        wl_surface_commit(o->surfaces[i]);
    }
    wl_display_roundtrip(o->display);
    widget_init(o->widget);

    while (o->active) {
        while (wl_display_prepare_read(o->display) != 0) {
            wl_display_dispatch_pending(o->display);
        }
        wl_display_flush(o->display);
        loop_run(o->loop);
    }

    if (_destroy()) {
        free(o);
        return 1;
    }

    return 0;
}

static void _layer_surface_configure(void *data,
                                   struct zwlr_layer_surface_v1 *surface,
                                   uint32_t serial, uint32_t w, uint32_t h) {
    (void)data; (void)w; (void)h;
    zwlr_layer_surface_v1_ack_configure(surface, serial);
}

static void _layer_surface_closed(void *data,
                                struct zwlr_layer_surface_v1 *surface) {
    // IMPLEMENT DESTROYING WIDGET
    (void)data;
    zwlr_layer_surface_v1_destroy(surface);
}

static void on_keyboard_callback(KeyboardData* d) {
    widget_keyboard(o->widget, d);
}

static void _toggle_widget() {
    o->widget->active = !o->widget->active;
    if (o->widget->active) {
        wl_surface_commit(o->widget->surface);
        wl_display_roundtrip(o->display);
    }
    else {
        o->keyboard->repeating = false;
        wl_surface_attach(o->widget->surface, NULL,0,0);
        wl_surface_damage(o->widget->surface, 0, 0, o->widget->width, o->widget->height);
        wl_surface_commit(o->widget->surface);
        wl_display_roundtrip(o->display);
    }
}

static struct wl_surface* _create_surface(size_t width, size_t height, int anchor, int layer) {
    struct wl_surface* s = wl_compositor_create_surface(o->compositor);
    struct zwlr_layer_surface_v1* ls = zwlr_layer_shell_v1_get_layer_surface(
        o->layer_shell, s, NULL, layer, "widget");
    static const struct zwlr_layer_surface_v1_listener ls_listener = (struct zwlr_layer_surface_v1_listener) {
        .configure = _layer_surface_configure, .closed = _layer_surface_closed
    };

    zwlr_layer_surface_v1_set_size(ls, width, height);
    zwlr_layer_surface_v1_set_anchor(ls, anchor);

    // TOP/BOTTOM are exclusive layers
    if (layer != ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY &&
        layer != ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND)
        zwlr_layer_surface_v1_set_exclusive_zone(ls, width);
    else
        zwlr_layer_surface_v1_set_exclusive_zone(ls, -1);

    zwlr_layer_surface_v1_set_keyboard_interactivity(ls,
            ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND);
    zwlr_layer_surface_v1_add_listener(ls, &ls_listener, o);

    // Add surface to overlord
    if (o->surfaces_size >= o->surfaces_capacity) {
        o->surfaces_capacity *= 2;
        o->surfaces = realloc(o->surfaces, o->surfaces_capacity * sizeof(struct wl_surface*));
        o->layer_surfaces = realloc(o->layer_surfaces, o->surfaces_capacity * sizeof(struct wl_layer_surfaces*));
    }

    o->surfaces[o->surfaces_size] = s;
    o->layer_surfaces[o->surfaces_size] = ls;
    o->surfaces_size++;

    return s;
}

static struct wl_registry_listener _registry_listener = { 
    .global_remove = NULL, .global = _registry_global
};

void _display_callback(int fd) {
    (void)fd;
    wl_display_read_events(o->display);
    wl_display_dispatch_pending(o->display);
}

void _step_callback(int fd) {
    uint64_t exp;
    read(fd, &exp, sizeof(exp));
    widget_step(o->widget);
    if (!widget_draw(o->widget)) {
        wl_display_flush(o->display);
    }
    wl_display_cancel_read(o->display);
}

void _repeat_callback(int fd) {
    uint64_t exp;
    read(fd, &exp, sizeof(exp));
    keyboard_repeat_key(o->keyboard);
    wl_display_cancel_read(o->display);
}

void _ipc_callback(int fd) {
    int client = accept(fd, NULL, NULL);
    if (client >= 0) {
        char buf[256];
        int n = read(client, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            if (strncmp(buf, "TOGGLE", 6) == 0) {
                _toggle_widget();
            }
        }
        close(client);
    }
    wl_display_cancel_read(o->display);
}

static int _init(int sock) {
    *o = (Overlord){
        .active = 1,
        .surfaces_capacity = 1,
        .surfaces = malloc(sizeof(struct wl_surface*)),
        .layer_surfaces = malloc(sizeof(struct zwlr_layer_surface_v1*)),
    };

    o->display = wl_display_connect(NULL);
    if (!o->display) {
        perror("Failed to connect to display");
        return 1;
    }

    // Setup registry
    o->registry = wl_display_get_registry(o->display);
    wl_registry_add_listener(o->registry, &_registry_listener, o);
    wl_display_roundtrip(o->display);
    if (!o->compositor || !o->shm || !o->layer_shell) {
        perror("Failed to load wayland interface");
        return 1;
    }

    o->keyboard = keyboard_attach(o->seat, on_keyboard_callback);

    // Setup widgets
    int anchor = 0;
    DRunData* data = malloc(sizeof(DRunData));
    Widget* w = widget_create(800, 400, o->shm, _toggle_widget);
    widget_attach_draw(w, drun_draw);
    widget_attach_init(w, drun_init);
    widget_attach_data(w, (void*)data);
    widget_attach_destroy(w, drun_destroy);
    widget_attach_keyboard(w, drun_on_key);
    widget_attach_surface(w, _create_surface(800, 400, anchor, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY));
    o->widget = w;

    // Create event loop
    o->loop = loop_create();
    loop_add_fd(o->loop, sock, _ipc_callback);
    loop_add_fd(o->loop, wl_display_get_fd(o->display), _display_callback);
    loop_add_fd(o->loop, o->keyboard->repeat_fd, _repeat_callback);
    loop_add_timer(o->loop, 17, _step_callback);

    return 0;
}

static int _destroy() {

    widget_destroy(o->widget);

    keyboard_destroy(o->keyboard);
    wl_seat_destroy(o->seat);
    wl_compositor_destroy(o->compositor);
    wl_shm_destroy(o->shm);
    wl_output_destroy(o->output);
    wl_registry_destroy(o->registry);
    wl_display_disconnect(o->display);
    return 0;
}


static void _registry_global(void *data, struct wl_registry *registry,
                           uint32_t name, const char *interface,
                           uint32_t version) {
    (void) data; (void) version;
    if (strcmp(interface, wl_compositor_interface.name) == 0)
        o->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    else if (strcmp(interface, wl_shm_interface.name) == 0)
        o->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0)
        o->layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
    else if (strcmp(interface, wl_output_interface.name) == 0 && !o->output)
        o->output = wl_registry_bind(registry, name, &wl_output_interface, 1);
    else if (strcmp(interface, wl_seat_interface.name) == 0)
        o->seat = wl_registry_bind(registry, name, &wl_seat_interface, 5);
}
