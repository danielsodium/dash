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
#ifdef DRUN
    else if (strcmp(interface, wl_seat_interface.name) == 0) {
        state->seat = wl_registry_bind(registry, name, &wl_seat_interface, 5);
    } 
#endif
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

#ifdef DRUN

static void key_handler(void *data, struct wl_keyboard *kbd, uint32_t serial,
                       uint32_t time, uint32_t key, uint32_t state) {
    (void)data; (void)kbd; (void)serial; (void)time;
    if (state != 1) return;
    if (key == KEY_ESC) exit(0);
    
    printf("Key: %d\n", key);
}

static void kbd_keymap(void *d, struct wl_keyboard *k, uint32_t f, int32_t fd, uint32_t s) { 
    (void)d; (void)k; (void)f; (void)s;
    close(fd); 
}
static void kbd_enter(void *d, struct wl_keyboard *k, uint32_t s, struct wl_surface *surf, struct wl_array *keys) {
    (void)d; (void)k; (void)s; (void)surf; (void)keys;
}
static void kbd_leave(void *d, struct wl_keyboard *k, uint32_t s, struct wl_surface *surf) {
    (void)d; (void)k; (void)s; (void)surf;
}
static void kbd_mods(void *d, struct wl_keyboard *k, uint32_t s, uint32_t dep, uint32_t lat, uint32_t lock, uint32_t grp) {
    (void)d; (void)k; (void)s; (void)dep; (void)lat; (void)lock; (void)grp;
}
static void kbd_repeat(void *d, struct wl_keyboard *k, int32_t rate, int32_t delay) {
    (void)d; (void)k; (void)rate; (void)delay;
}

static void handle_keyboard(void *data, struct wl_seat *seat, uint32_t caps) {
    (void)seat;
    Window* wl = data;
    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !wl->keyboard) {
        wl->keyboard = wl_seat_get_keyboard(wl->seat);
        wl_keyboard_add_listener(wl->keyboard, wl->keyboard_listener, NULL);
    }
}
static void seat_name(void *data, struct wl_seat *seat, const char *name) {
    (void)data; (void)seat; (void)name;
}

#endif

Window* window_init() {
    Window* wl;

    wl = malloc(sizeof(Window));
    wl->registry_listener = malloc(sizeof(struct wl_registry_listener));
    wl->layer_surface_listener = malloc(sizeof(struct zwlr_layer_surface_v1_listener));
#ifdef DRUN
    wl->keyboard = NULL;
    wl->seat_listener = malloc(sizeof(struct wl_seat_listener));
    wl->seat_listener->capabilities = handle_keyboard,
    wl->seat_listener->name = seat_name,

    wl->keyboard_listener = malloc(sizeof(struct wl_keyboard_listener));
    wl->keyboard_listener->keymap = kbd_keymap,
    wl->keyboard_listener->enter = kbd_enter,
    wl->keyboard_listener->leave = kbd_leave,
    wl->keyboard_listener->key = key_handler,
    wl->keyboard_listener->modifiers = kbd_mods,
    wl->keyboard_listener->repeat_info = kbd_repeat,
#endif
    wl->compositor = NULL;
    wl->shm = NULL;
    wl->layer_shell = NULL;
    wl->output = NULL;
    wl->active = 1;


    wl->display = wl_display_connect(NULL);
    if (!wl->display) {
        perror("Failed to connect to display");
        return NULL;
    }

    wl->registry = wl_display_get_registry(wl->display);
    wl->registry_listener->global = registry_global,
    wl->registry_listener->global_remove = NULL;

    wl_registry_add_listener(wl->registry, wl->registry_listener, wl);
    wl_display_roundtrip(wl->display);

#ifdef DRUN
    wl_seat_add_listener(wl->seat, wl->seat_listener, wl);
    wl_display_roundtrip(wl->display);
#endif

    if (!wl->compositor || !wl->shm || !wl->layer_shell) {
        perror("Failed to load wayland interface");
        return NULL;
    }

    wl->surface = wl_compositor_create_surface(wl->compositor);
    wl->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        wl->layer_shell, wl->surface, wl->output, 
#ifdef DRUN
            ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY
#else
            ZWLR_LAYER_SHELL_V1_LAYER_TOP
#endif
        , "panel");

    zwlr_layer_surface_v1_set_size(wl->layer_surface, WIDTH_PIXELS, HEIGHT_PIXELS);
    zwlr_layer_surface_v1_set_anchor(wl->layer_surface,
        ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
#ifdef DRUN
        | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
#endif
        );
    zwlr_layer_surface_v1_set_exclusive_zone(wl->layer_surface, WIDTH_PIXELS);
    zwlr_layer_surface_v1_set_keyboard_interactivity(wl->layer_surface,
#ifdef DRUN
            ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE
#else
            ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE
#endif
    );

    wl->layer_surface_listener->configure = layer_surface_configure;
    wl->layer_surface_listener->closed = layer_surface_closed;
    zwlr_layer_surface_v1_add_listener(wl->layer_surface, wl->layer_surface_listener, wl);


    wl_surface_commit(wl->surface);
    wl_display_roundtrip(wl->display);

    wl->canvas = canvas_init(wl->shm);
    return wl;
}

void window_draw(Window* win) {
    char buffer[1024];

    draw_start(win->canvas);
#ifdef DRUN
    drun(buffer);
    draw_string(win->canvas, buffer, ANCHOR_CENTER);
#else
    clock_widget(buffer);
    draw_string(win->canvas, buffer, ANCHOR_TOP);
    system_widget(buffer);
    draw_string(win->canvas, buffer, ANCHOR_BOTTOM);
#endif
    draw_to_buffer(win->canvas);

    wl_surface_attach(win->surface, win->canvas->buffer, 0, 0);
    wl_surface_damage(win->surface, 0, 0, WIDTH_PIXELS, HEIGHT_PIXELS);
    wl_surface_commit(win->surface);
}

void window_handle_events(Window* win) {
    wl_display_dispatch(win->display);
    wl_display_flush(win->display);
}

void window_destroy(Window* win) {
    canvas_destroy(win->canvas);
    zwlr_layer_surface_v1_destroy(win->layer_surface);
    wl_surface_destroy(win->surface);
    wl_display_disconnect(win->display);
    free(win->registry_listener);
    free(win);
}
