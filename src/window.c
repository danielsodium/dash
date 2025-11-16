#include "window.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <linux/input-event-codes.h>

#include "config.h"
#include "draw.h"
#include "widgets.h"

static void _registry_global(void *data, struct wl_registry *registry,
                           uint32_t name, const char *interface,
                           uint32_t version) {
    (void) version;
    Window* w = data;
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

static void _layer_surface_configure(void *data,
                                   struct zwlr_layer_surface_v1 *surface,
                                   uint32_t serial, uint32_t w, uint32_t h) {
    (void)data; (void)w; (void)h;
    zwlr_layer_surface_v1_ack_configure(surface, serial);
}

static void _layer_surface_closed(void *data,
                                struct zwlr_layer_surface_v1 *surface) {
    (void)surface;
    ((Window*) data)->active = 0;
}


static void _kb_key(void *data, struct wl_keyboard *kbd, uint32_t serial,
                       uint32_t time, uint32_t key, uint32_t state) {
    (void)data; (void)kbd; (void)serial; (void)time;
    if (state != 1) return;
    if (key == KEY_ESC) exit(0);
    
    printf("Key: %d\n", key);
}

static void _kb_keymap(void *d, struct wl_keyboard *k, uint32_t f, int32_t fd, uint32_t s) { 
    (void)d; (void)k; (void)f; (void)s;
    close(fd); 
}
static void _kb_enter(void *d, struct wl_keyboard *k, uint32_t s, struct wl_surface *surf, struct wl_array *keys) {
    (void)d; (void)k; (void)s; (void)surf; (void)keys;
}
static void _kb_leave(void *d, struct wl_keyboard *k, uint32_t s, struct wl_surface *surf) {
    (void)d; (void)k; (void)s; (void)surf;
}
static void _kb_mods(void *d, struct wl_keyboard *k, uint32_t s, uint32_t dep, uint32_t lat, uint32_t lock, uint32_t grp) {
    (void)d; (void)k; (void)s; (void)dep; (void)lat; (void)lock; (void)grp;
}
static void _kb_repeat(void *d, struct wl_keyboard *k, int32_t rate, int32_t delay) {
    (void)d; (void)k; (void)rate; (void)delay;
}

static void _handle_keyboard(void *data, struct wl_seat *seat, uint32_t caps) {
    (void)seat;
    Window* w = data;
    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !w->keyboard) {
        w->keyboard = wl_seat_get_keyboard(w->seat);
        wl_keyboard_add_listener(w->keyboard, w->keyboard_listener, NULL);
    }
}
static void _seat_name(void *data, struct wl_seat *seat, const char *name) {
    (void)data; (void)seat; (void)name;
}

Window* window_create(int width, int height, int anchor, int layer) {
    Window* w;

    w = malloc(sizeof(Window));
    w->registry_listener = malloc(sizeof(struct wl_registry_listener));
    w->layer_surface_listener = malloc(sizeof(struct zwlr_layer_surface_v1_listener));

    w->display = wl_display_connect(NULL);
    if (!w->display) {
        perror("Failed to connect to display");
        return NULL;
    }

    // Setup registry
    w->output = NULL; w->compositor = NULL;
    w->shm = NULL; w->layer_shell = NULL;
    w->registry = wl_display_get_registry(w->display);
    *(w->registry_listener) = (struct wl_registry_listener) { 
        .global_remove = NULL, .global = _registry_global
    };
    wl_registry_add_listener(w->registry, w->registry_listener, w);
    wl_display_roundtrip(w->display);
    if (!w->compositor || !w->shm || !w->layer_shell) {
        perror("Failed to load wayland interface");
        return NULL;
    }

    // Setup surface
    w->surface = wl_compositor_create_surface(w->compositor);
    w->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        w->layer_shell, w->surface, w->output, layer, "window");
    zwlr_layer_surface_v1_set_size(w->layer_surface, width, height);
    zwlr_layer_surface_v1_set_anchor(w->layer_surface, anchor);
    zwlr_layer_surface_v1_set_exclusive_zone(w->layer_surface, width);
    zwlr_layer_surface_v1_set_keyboard_interactivity(w->layer_surface,
            ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE);
    *(w->layer_surface_listener) = (struct zwlr_layer_surface_v1_listener) {
        .configure = _layer_surface_configure, .closed = _layer_surface_closed
    };
    zwlr_layer_surface_v1_add_listener(w->layer_surface, w->layer_surface_listener, w);

    return w;
}

void window_attach_keyboard_listener(Window* w) {
    zwlr_layer_surface_v1_set_keyboard_interactivity(w->layer_surface,
            ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND);

    w->keyboard = NULL;
    w->seat_listener = malloc(sizeof(struct wl_seat_listener));
    *(w->seat_listener) = (struct wl_seat_listener){
        .capabilities = _handle_keyboard, .name = _seat_name
    };

    wl_seat_add_listener(w->seat, w->seat_listener, w);

    w->keyboard_listener = malloc(sizeof(struct wl_keyboard_listener));
    *(w->keyboard_listener) = (struct wl_keyboard_listener){
        .keymap = _kb_keymap,
        .leave =  _kb_leave,
        .enter =  _kb_enter,
        .key = _kb_key,
        .modifiers = _kb_mods,
        .repeat_info = _kb_repeat
    };

    wl_display_roundtrip(w->display);
}

void window_commit(Window* w) {
    wl_surface_commit(w->surface);
    wl_display_roundtrip(w->display);
    w->active = 1;

    w->canvas = canvas_init(w->shm);
}

void window_destroy(Window* w) {
    zwlr_layer_surface_v1_destroy(w->layer_surface);
    wl_surface_destroy(w->surface);
    wl_compositor_destroy(w->compositor);
    zwlr_layer_shell_v1_destroy(w->layer_shell);
    wl_shm_destroy(w->shm);
    wl_output_destroy(w->output);
    wl_seat_destroy(w->seat);
    wl_registry_destroy(w->registry);
    wl_display_disconnect(w->display);
    free(w->layer_surface_listener);
    free(w->registry_listener);
    free(w);
}
void window_draw(Window* win) {
    char buffer[1024];

    draw_start(win->canvas);
    strcpy(buffer, "HELLO\nWORLD\n");
    draw_string(win->canvas, buffer, ANCHOR_TOP);
    draw_to_buffer(win->canvas);

    wl_surface_attach(win->surface, win->canvas->buffer, 0, 0);
    wl_surface_damage(win->surface, 0, 0, WIDTH_PIXELS, HEIGHT_PIXELS);
    wl_surface_commit(win->surface);
}

void window_handle_events(Window* win) {
    wl_display_roundtrip(win->display);
    wl_display_dispatch_pending(win->display);
    wl_display_flush(win->display);
}

