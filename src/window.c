#include "window.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <errno.h>
#include <sys/timerfd.h>

#include "widgets.h"

// Callback functions
static void _registry_global(void *data, struct wl_registry *registry, 
                             uint32_t name, const char *interface, uint32_t version);
static void _layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *surface, 
                                     uint32_t serial, uint32_t w, uint32_t h);
static void _layer_surface_closed(void *data, struct zwlr_layer_surface_v1 *surface);
static void _kb_key(void *data, struct wl_keyboard *kbd, uint32_t serial,
                    uint32_t time, uint32_t key, uint32_t state);
static void _kb_keymap(void *d, struct wl_keyboard *k, uint32_t f, int32_t fd, 
                       uint32_t s);
static void _kb_enter(void *d, struct wl_keyboard *k, uint32_t s, 
                      struct wl_surface *surf, struct wl_array *keys);
static void _kb_leave(void *d, struct wl_keyboard *k, uint32_t s, 
                      struct wl_surface *surf);
static void _kb_mods(void *d, struct wl_keyboard *k, uint32_t s, uint32_t dep, 
                     uint32_t lat, uint32_t lock, uint32_t grp);
static void _kb_repeat(void *d, struct wl_keyboard *k, int32_t rate, int32_t delay);
static void _handle_keyboard(void *data, struct wl_seat *seat, uint32_t caps);
static void _seat_name(void *data, struct wl_seat *seat, const char *name);
static int _timer_create(long interval_ms);

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
    
    // Initalize
    w->output = NULL; 
    w->compositor = NULL;
    w->shm = NULL; 
    w->layer_shell = NULL;
    w->keyboard_attached = 0;

    // Setup registry
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
    zwlr_layer_surface_v1_set_size(w->layer_surface, w->width=width, w->height=height);
    zwlr_layer_surface_v1_set_anchor(w->layer_surface, anchor);
    if (layer != ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY)
        zwlr_layer_surface_v1_set_exclusive_zone(w->layer_surface, width);
    else
        zwlr_layer_surface_v1_set_exclusive_zone(w->layer_surface, -1);
    zwlr_layer_surface_v1_set_keyboard_interactivity(w->layer_surface,
            ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE);
    *(w->layer_surface_listener) = (struct zwlr_layer_surface_v1_listener) {
        .configure = _layer_surface_configure, .closed = _layer_surface_closed
    };
    zwlr_layer_surface_v1_add_listener(w->layer_surface, w->layer_surface_listener, w);

    return w;
}

void window_attach_keyboard_listener(Window* w, void(*on_key_func)(xkb_keysym_t*, int*, void*)) {
    zwlr_layer_surface_v1_set_keyboard_interactivity(w->layer_surface,
            ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND);
    w->keyboard = NULL;
    w->keyboard_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
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

    w->keyboard_attached = 1;
    w->on_key = on_key_func;
    wl_display_roundtrip(w->display);
}

void window_attach_step(Window* w, int interval, void(*step_func)(int*, void*)) {
    w->step_attached = 1;
    w->step_interval = interval;
    w->step = step_func;
}

void window_commit(Window* w, int(*draw_func)(cairo_t*, int*, void*)) {
    wl_surface_commit(w->surface);
    wl_display_roundtrip(w->display);
    w->canvas = canvas_create(w->width,  w->height, w->shm);
    w->active = 1;
    w->draw = draw_func;

    // Create event loop
    w->epoll_fd = epoll_create1(0);
    if (w->epoll_fd == -1) {
        perror("Failed to create epoll");
        return;
    }

    w->display_fd = wl_display_get_fd(w->display);
    struct epoll_event ev = {0};
    ev.events = EPOLLIN;
    ev.data.fd = w->display_fd;
    if (epoll_ctl(w->epoll_fd, EPOLL_CTL_ADD, w->display_fd, &ev) == -1) {
        perror("Failed to epoll wayland display");
        return;
    }

    if (w->step_attached) {
        w->step_fd = _timer_create(w->step_interval);
        if (w->step_fd == -1) {
            perror("Failed to create step timer");
            return;
        }
        ev.events = EPOLLIN;
        ev.data.fd = w->step_fd;
        if (epoll_ctl(w->epoll_fd, EPOLL_CTL_ADD, w->step_fd, &ev) == -1) {
            perror("Failed to epoll step");
            return;
        }
    }

    // DRAW
    w->draw_fd = _timer_create(150);
    if (w->draw_fd == -1) {
        perror("Failed to create draw timer");
        return;
    }
    ev.events = EPOLLIN;
    ev.data.fd = w->draw_fd;
    if (epoll_ctl(w->epoll_fd, EPOLL_CTL_ADD, w->draw_fd, &ev) == -1) {
        perror("Failed to epoll draw");
        return;
    }

}

void window_draw(Window* w) {
    int refresh = w->draw(w->canvas->cairo, &w->active, w->data);
    if (!refresh) return;
    wl_surface_attach(w->surface, w->canvas->buffer, 0, 0);
    wl_surface_damage(w->surface, 0, 0, w->width, w->height);
    wl_surface_commit(w->surface);
    wl_display_flush(w->display);
}

void window_handle_events(Window* w) {
    wl_display_roundtrip(w->display);
    wl_display_dispatch_pending(w->display);
}

void* window_get_data(Window* w) {
    return w->data;
}
void window_set_data(Window* w, void* data) {
    w->data = data;
}

void window_loop(Window* w) {
    struct epoll_event events[10];
    while (w->active) {
        while (wl_display_prepare_read(w->display) != 0) {
            wl_display_dispatch_pending(w->display);
        }
        wl_display_flush(w->display);

        int nfds = epoll_wait(w->epoll_fd, events, 10, -1);
        if (nfds == -1) {
            if (errno == EINTR) {
                wl_display_cancel_read(w->display);
                continue;
            }
            perror("Failed to wait epoll");
            wl_display_cancel_read(w->display);
            break;
        }

        int wl_has_data = 0;
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == w->display_fd) {
                wl_has_data = 1;
                break;
            }
        }

        if (wl_has_data) {
            wl_display_read_events(w->display);
            wl_display_dispatch_pending(w->display);
        } else {
            wl_display_cancel_read(w->display);
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == w->step_fd) {
                uint64_t exp;
                read(w->step_fd, &exp, sizeof(exp));
                w->step(&w->active, w->data);
            } else if (events[i].data.fd == w->draw_fd) {
                uint64_t exp;
                read(w->draw_fd, &exp, sizeof(exp));
                window_draw(w);
            }
        }

    }

}

void window_destroy(Window* w) {

    if (w->step_attached && w->step_fd != -1) {
        close(w->step_fd);
    }
    if (w->draw_fd != -1) {
        close(w->draw_fd);
    }
    if (w->epoll_fd != -1) {
        close(w->epoll_fd);
    }
    if (w->keyboard_attached) {
        xkb_state_unref(w->keyboard_state);
        xkb_keymap_unref(w->keyboard_keymap);
        xkb_context_unref(w->keyboard_context);
        wl_keyboard_destroy(w->keyboard);
        free(w->keyboard_listener);
        free(w->seat_listener);
    }
    canvas_destroy(w->canvas);
    zwlr_layer_surface_v1_destroy(w->layer_surface);
    wl_surface_destroy(w->surface);
    wl_seat_destroy(w->seat);
    wl_compositor_destroy(w->compositor);
    zwlr_layer_shell_v1_destroy(w->layer_shell);
    wl_shm_destroy(w->shm);
    wl_output_destroy(w->output);
    wl_registry_destroy(w->registry);
    wl_display_disconnect(w->display);
    free(w->layer_surface_listener);
    free(w->registry_listener);
    free(w);
}

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
    Window* w = data;
    if (state != WL_KEYBOARD_KEY_STATE_PRESSED)
        return;

    xkb_keysym_t sym = xkb_state_key_get_one_sym(w->keyboard_state, key+8);
    w->on_key(&sym, &w->active, w->data);
    window_draw(w);
}

static void _kb_keymap(void *d, struct wl_keyboard *k, uint32_t f, int32_t fd, uint32_t s) { 
    (void)k;
    char* map_str;
    Window* w;

    w = d;
    if (f != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd); 
        return;
    }
    map_str = mmap(NULL, s, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    if (map_str == MAP_FAILED) return;

    w->keyboard_keymap = xkb_keymap_new_from_string(w->keyboard_context,
                                                    map_str,
                                                    XKB_KEYMAP_FORMAT_TEXT_V1,
                                                    XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(map_str, s);
    if (!w->keyboard_keymap) return;
    w->keyboard_state = xkb_state_new(w->keyboard_keymap);
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
        wl_keyboard_add_listener(w->keyboard, w->keyboard_listener, w);
    }
}

static void _seat_name(void *data, struct wl_seat *seat, const char *name) {
    (void)data; (void)seat; (void)name;
}

static int _timer_create(long interval_ms) {
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (fd == -1) {
        perror("Failed to create timer");
        return -1;
    }

    struct itimerspec spec = {0};
    spec.it_value.tv_sec = interval_ms / 1000;
    spec.it_value.tv_nsec = (interval_ms % 1000) * 1000000;
    spec.it_interval = spec.it_value;
    
    if (timerfd_settime(fd, 0, &spec, NULL) == -1) {
        perror("Failed to set timer");
        close(fd);
        return -1;
    }

    return fd;
}
