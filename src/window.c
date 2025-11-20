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
#include "keyboard.h"

#define CREATE_EPOLL(fd_name)\
    ev.events = EPOLLIN;\
    ev.data.fd = w->fd_name;\
    if (epoll_ctl(w->epoll_fd, EPOLL_CTL_ADD, w->fd_name, &ev) == -1) {\
        perror("Failed to epoll fd_name");\
        return;\
    }

#define CREATE_SYSTEM_EVENT(event_name, event_data) \
    void on_##event_name##_callback(Window* w, event_data* data) { \
        if (w->on_##event_name(data, &w->active, w->data)) { \
            window_draw(w); \
        } \
    } \
    void window_attach_##event_name##_listener(Window* w, \
        int(*on_##event_name)(event_data*, int*, void*)) { \
        w->event_name##_attached = 1; \
        w->on_##event_name = on_##event_name; \
        w->event_name = event_name##_attach(w, w->seat, on_##event_name##_callback); \
    }

// Callback functions
static void _registry_global(void *data, struct wl_registry *registry, 
                             uint32_t name, const char *interface, uint32_t version);
static void _layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *surface, 
                                     uint32_t serial, uint32_t w, uint32_t h);
static void _layer_surface_closed(void *data, struct zwlr_layer_surface_v1 *surface);
static int _timer_create(long interval_ms);
static void _window_commit(Window* w);
static void _window_loop(Window* w);
static void _window_destroy(Window* w);

Window* window_create(int width, int height, int anchor, int layer) {
    Window* w = malloc(sizeof(Window));
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
    w->epoll_fd = -1;
    w->display_fd = -1;
    w->step_fd = -1;
    w->repeat_fd = -1;

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
    zwlr_layer_surface_v1_set_keyboard_interactivity(w->layer_surface,
            ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND);
    *(w->layer_surface_listener) = (struct zwlr_layer_surface_v1_listener) {
        .configure = _layer_surface_configure, .closed = _layer_surface_closed
    };
    zwlr_layer_surface_v1_add_listener(w->layer_surface, w->layer_surface_listener, w);

    return w;
}

CREATE_SYSTEM_EVENT(keyboard, KeyboardData)

void window_attach_step(Window* w, int interval, int(*step_func)(int*, void*)) {
    w->step_attached = 1;
    w->step_interval = interval;
    w->step = step_func;
}

void window_attach_draw(Window* w, void(*draw_func)(cairo_t*, int*, void*)) {
    w->draw_attached = 1;
    w->draw = draw_func;
}

void window_attach_destroy(Window* w, void(*destroy)(void*)) {
    w->destroy_attached = 1;
    w->destroy = destroy;
}

void window_attach_init(Window* w, void (*init)(cairo_t*, void*)) {
    w->init_attached = 1;
    w->init = init;
}
void window_handle_events(Window* w) {
    wl_display_roundtrip(w->display);
    wl_display_dispatch_pending(w->display);
}

void window_attach_data(Window* w, void* data) {
    w->data = data;
}

void window_run(Window* w) {
    _window_commit(w);
    if (w->init_attached)
        w->init(w->canvas->cairo, w->data);
    if (w->draw_attached)
        window_draw(w);
    _window_loop(w);
    _window_destroy(w);
}

void window_draw(Window* w) {
    if (!w->draw_attached) return;
    w->draw(w->canvas->cairo, &w->active, w->data);
    wl_surface_attach(w->surface, w->canvas->buffer, 0, 0);
    wl_surface_damage(w->surface, 0, 0, w->width, w->height);
    wl_surface_commit(w->surface);
    wl_display_flush(w->display);
}

static void _window_commit(Window* w) { 
    wl_surface_commit(w->surface);
    wl_display_roundtrip(w->display);
    w->canvas = canvas_create(w->width,  w->height, w->shm);
    w->active = 1;

    // Create event loop
    w->epoll_fd = epoll_create1(0);
    if (w->epoll_fd == -1) {
        perror("Failed to create epoll");
        return;
    }
    struct epoll_event ev = {0};
    w->display_fd = wl_display_get_fd(w->display);
    CREATE_EPOLL(display_fd);

    if (w->step_attached) {
        w->step_fd = _timer_create(w->step_interval);
        if (w->step_fd == -1) {
            perror("Failed to create step timer");
            return;
        }
        CREATE_EPOLL(step_fd);
    }
    
    if (w->keyboard_attached) {
        w->repeat_fd = w->keyboard->repeat_fd;
        CREATE_EPOLL(repeat_fd);
    }


}

static void _window_loop(Window* w) {
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

        uint64_t exp;
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == w->step_fd) {
                read(w->step_fd, &exp, sizeof(exp));
                int should_draw = w->step(&w->active, w->data);
                if (should_draw) window_draw(w);
            } else if (events[i].data.fd == w->repeat_fd) {
                read(w->keyboard->repeat_fd, &exp, sizeof(exp));
                if (w->keyboard->repeating) {
                    KeyboardData event_data = {
                        .event = KEYBOARD_EVENT_KEY_REPEAT,
                        .key = &w->keyboard->last_pressed
                    };
                    if (w->on_keyboard(&event_data, &w->active, w->data)) {
                        window_draw(w);
                    }
                }
            }
        }
    }
}

static void _window_destroy(Window* w) {
    if (w->destroy_attached) 
        w->destroy(w->data);

    if (w->step_fd != -1)
        close(w->step_fd);
    if (w->epoll_fd != -1)
        close(w->epoll_fd);
    if (w->repeat_fd != -1)
        close(w->repeat_fd);
    if (w->keyboard_attached)
        keyboard_destroy(w->keyboard);

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
