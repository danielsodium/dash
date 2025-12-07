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
#include "wayland.h"
#include "keyboard.h"

// SINGLETON OVERLORD
static Overlord* o = NULL;

static void on_keyboard_callback(KeyboardData* d) {
    widget_keyboard(o->widget, d);
}

static void _toggle_widget() {
    o->widget->active = !o->widget->active;
    if (o->widget->active) {
        wayland_commit(o->widget->surface);
        wayland_roundtrip();
    } else {
        o->keyboard->repeating = false;
        wayland_hide_surface(o->widget->surface);
   }
    wayland_flush();
}

void _display_callback(int fd) {
    (void)fd;
    wayland_display_events();
}

void _step_callback(int fd) {
    wayland_cancel_read();
    uint64_t exp;
    read(fd, &exp, sizeof(exp));
    widget_step(o->widget);
    if (!widget_draw(o->widget)) {
        wayland_flush();
    }
}

void _repeat_callback(int fd) {
    wayland_cancel_read();
    uint64_t exp;
    read(fd, &exp, sizeof(exp));
    keyboard_repeat_key(o->keyboard);
}

void _ipc_callback(int fd) {
    wayland_cancel_read();
    int client = accept(fd, NULL, NULL);
    if (client >= 0) {
char buf[256];
        int n = read(client, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            if (strncmp(buf, "TOGGLE", 6) == 0) {
                _toggle_widget();
                wayland_flush();
            }
        }
        close(client);
    }
}


int overlord_run(int sock) {
    o = calloc(1, sizeof(Overlord));
    if (!o) {
        perror("Failed to allocate the overlord");
        return 1;
    }

    if (wayland_init()) {
        free(o);
        return 1;
    }

    o->active = 1;
    o->keyboard = keyboard_attach(wayland_seat(), on_keyboard_callback);

    // Setup widgets
    int anchor = 0;
    DRunData* data = malloc(sizeof(DRunData));
    WidgetOps* w_ops = malloc(sizeof(WidgetOps));
    *w_ops = (WidgetOps) {
        .init = drun_init,
        .draw = drun_draw,
        .step = NULL,
        .keyboard = drun_on_key,
        .destroy = drun_destroy,
        .toggle = _toggle_widget
    };
    Widget* w = widget_create(800, 400, (void*)data, wayland_shm(), w_ops, wayland_create_surface(800, 400, anchor, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY));
    o->widget = w;

    // Create event loop
    o->loop = loop_create();
    loop_add_fd(o->loop, sock, _ipc_callback);
    loop_add_fd(o->loop, wayland_display_fd(), _display_callback);
    loop_add_fd(o->loop, o->keyboard->repeat_fd, _repeat_callback);
    loop_add_timer(o->loop, 17, _step_callback);

    // LOOP
    wayland_commit_surfaces();
    widget_init(o->widget);
    while (o->active) {
        wayland_prepare_display();
        loop_run(o->loop);
    }

    return 0;
}
