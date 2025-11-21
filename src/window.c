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
        return 1;\
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
static int _timer_create(long interval_ms);

int overlord_run() {
    Overlord o;

    if (_overlord_init(&o)) return 1;
    if (_overlord_loop(&o)) return 1;;
    if (_overlord_destroy(&o)) return 1:

    return 0;
}


int _overlord_init(Overlord* o);
    *o = (Overlord){
        .active = 1,
        .step_interval = -1,
        .epoll_fd = -1,
        .display_fd = -1,
        .step_fd = -1,
        .repeat_fd = -1,
        .display = NULL,
        .registry = NULL,
        .registry_listener = (struct wl_registry_listener) { 
            .global_remove = NULL, .global = _registry_global
        },
        .compositor = NULL,
        .shm = NULL,
        .layer_shell = NULL,
        .output = NULL,
        .seat = NULL,
        .keyboard = NULL
    };

    o->display = wl_display_connect(NULL);
    if (!o->display) {
        perror("Failed to connect to display");
        return 1;
    }

    // Setup registry
    o->registry = wl_display_get_registry(o->display);
    wl_registry_add_listener(o->registry, o->registry_listener, o);
    wl_display_roundtrip(o->display);
    if (!o->compositor || !o->shm || !o->layer_shell) {
        perror("Failed to load wayland interface");
        return 1;
    }

    // Create event loop
    o->epoll_fd = epoll_create1(0);
    if (o->epoll_fd == -1) {
        perror("Failed to create epoll");
        return 1;
    }
    struct epoll_event ev = {0};

    // EVENTS
    o->display_fd = wl_display_get_fd(o->display);
    CREATE_EPOLL(display_fd);

    o->repeat_fd = o->keyboard->repeat_fd;
    CREATE_EPOLL(repeat_fd);

    return 0;
}

CREATE_SYSTEM_EVENT(keyboard, KeyboardData);

static int _overlord_loop(Overlord* o) {
    while (o->active) {
        while (wl_display_prepare_read(o->display) != 0) {
            wl_display_dispatch_pending(o->display);
        }
        wl_display_flush(o->display);

        int nfds = epoll_wait(o->epoll_fd, o->events, 10, -1);
        if (nfds == -1) {
            wl_display_cancel_read(o->display);
            if (errno == EINTR) continue;
            perror("Failed to wait epoll");
            return 1;
        }

        int wl_has_data = 0;
        for (int i = 0; i < nfds; i++) {
            if (o->events[i].data.fd == o->display_fd) {
                wl_has_data = 1;
                break;
            }
        }

        if (wl_has_data) {
            wl_display_read_events(o->display);
            wl_display_dispatch_pending(o->display);
        } else {
            wl_display_cancel_read(o->display);
        }

        uint64_t exp;
        for (int i = 0; i < nfds; i++) {
            if (o->events[i].data.fd == o->step_fd) {
                read(o->step_fd, &exp, sizeof(exp));
                _overlord_draw(o);
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
    return 0;
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
