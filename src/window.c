#include "window.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/timerfd.h>

#include "widget.h"
#include "drun.h"
#include "keyboard.h"

// SINGLETON OVERLORD
static Overlord* o = NULL;

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
static int _overlord_init(int sock);
static int _overlord_loop();
static int _overlord_destroy();

int overlord_run(int sock) {
    o = malloc(sizeof(Overlord));
    if (!o) {
        perror("Failed to allocated the overlord");
        return 1;
    }

    if (_overlord_init(sock)) {
        free(o);
        return 1;
    }
    if (_overlord_loop()) {
        free(o);
        return 1;
    }
    if (_overlord_destroy()) {
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

void on_keyboard_callback(KeyboardData* d) {
    widget_keyboard(o->widget, d);
}

void overlord_toggle_widget() {
    o->widget->active = !o->widget->active;
    if (o->widget->active) {
        wl_surface_commit(o->widget->surface);
        wl_display_roundtrip(o->display);
        widget_draw(o->widget);
        wl_display_flush(o->display);
    }
}

static struct wl_surface* _overlord_create_surface(size_t width, size_t height, int anchor, int layer) {
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

static int _overlord_init(int sock) {
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
        .surfaces_size = 0,
        .surfaces_capacity = 1,
        .surfaces = malloc(sizeof(struct wl_surface*)),
        .layer_surfaces = malloc(sizeof(struct zwlr_layer_surface_v1*)),
        .ipc_fd = sock
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

    o->keyboard = keyboard_attach(o->seat, on_keyboard_callback);


    // Setup widget
    int anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                 ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | 
                 ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
    DRunData* data = malloc(sizeof(DRunData));
    Widget* w = widget_create(180, 1440, o->shm, overlord_toggle_widget);
    widget_attach_draw(w, drun_draw);
    widget_attach_init(w, drun_init);
    widget_attach_data(w, (void*)data);
    widget_attach_destroy(w, drun_destroy);
    widget_attach_keyboard(w, drun_on_key);
    widget_attach_surface(w, _overlord_create_surface(180, 1440, anchor, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY));
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

    o->step_interval = 17;
    o->step_fd = _timer_create(o->step_interval);
    if (o->step_fd == -1) {
        perror("Failed to create step timer");
        return 1;
    }
    CREATE_EPOLL(step_fd);
    CREATE_EPOLL(ipc_fd);

    return 0;
}

static int _overlord_loop() {
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
                widget_step(o->widget);
                if (!widget_draw(o->widget)) {
                    wl_display_flush(o->display);
                }
            } else if (o->events[i].data.fd == o->repeat_fd) {
                read(o->keyboard->repeat_fd, &exp, sizeof(exp));
                keyboard_repeat_key(o->keyboard);
            } else if (o->events[i].data.fd == o->ipc_fd) {
                printf("Recieved IPC signal\n");
                int client = accept(o->ipc_fd, NULL, NULL);
                if (client >= 0) {
                    char buf[256];
                    int n = read(client, buf, sizeof(buf) - 1);
                    if (n > 0) {
                        buf[n] = '\0';
                        if (strncmp(buf, "TOGGLE", 6) == 0) {
                            overlord_toggle_widget();
                        }
                    }
                    close(client);
                }
            }
        }
    }
    return 0;
}

static int _overlord_destroy() {
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
        close(fd);
        return -1;
    }

    return fd;
}
