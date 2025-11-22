#include "window.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/timerfd.h>

#include "widget.h"
#include "drun.h"
#include "keyboard.h"

#define CREATE_EPOLL(fd_name)\
    ev.events = EPOLLIN;\
    ev.data.fd = o->fd_name;\
    if (epoll_ctl(o->epoll_fd, EPOLL_CTL_ADD, o->fd_name, &ev) == -1) {\
        perror("Failed to epoll fd_name");\
        return 1;\
    }

// Callback functions
static void _registry_global(void *data, struct wl_registry *registry, 
                             uint32_t name, const char *interface, uint32_t version);
static int _timer_create(long interval_ms);
static int _overlord_init(Overlord* o);
static int _overlord_loop(Overlord* o);
static int _overlord_destroy(Overlord* o);

int overlord_run() {
    Overlord o;

    if (_overlord_init(&o)) return 1;
    if (_overlord_loop(&o)) return 1;
    if (_overlord_destroy(&o)) return 1;

    return 0;
}


static int _overlord_init(Overlord* o) {
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
        .keyboard = NULL,
        .widget = NULL,
    };

    o->display = wl_display_connect(NULL);
    if (!o->display) {
        perror("Failed to connect to display");
        return 1;
    }

    // Setup registry
    o->registry = wl_display_get_registry(o->display);
    wl_registry_add_listener(o->registry, &o->registry_listener, o);
    wl_display_roundtrip(o->display);
    if (!o->compositor || !o->shm || !o->layer_shell) {
        perror("Failed to load wayland interface");
        return 1;
    }

    // Setup widget
    int anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                 ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | 
                 ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
    Widget* w = widget_create(180, 1440, anchor, ZWLR_LAYER_SHELL_V1_LAYER_TOP);
    DRunData* data = malloc(sizeof(DRunData));
    widget_attach_draw(w, drun_draw);
    widget_attach_init(w, drun_init);
    widget_attach_data(w, (void*)data);
    widget_attach_destroy(w, drun_destroy);
    widget_attach_keyboard_listener(w, drun_on_key);
    o->widget = w;

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
                // LATER LOOP THRU WIDGETS HERE
                if (o->widget->step_attached)
                    o->active = !widget_run_step(o->widget);
            } else if (o->events[i].data.fd == o->repeat_fd) {
                read(o->keyboard->repeat_fd, &exp, sizeof(exp));
                keyboard_repeat_key(o->keyboard);
            }
        }
    }
    return 0;
}

static int _overlord_destroy(Overlord* o) {
    if (o->step_fd != -1)
        close(o->step_fd);
    if (o->epoll_fd != -1)
        close(o->epoll_fd);
    if (o->repeat_fd != -1)
        close(o->repeat_fd);
    
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
    (void) version;
    Overlord* o = data;
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
