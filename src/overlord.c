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
#include "bar.h"
#include "wallpaper.h"
#include "wayland.h"
#include "keyboard.h"

// SINGLETON OVERLORD
static Overlord* o = NULL;

static void on_keyboard_callback(KeyboardData* d) {
    for (size_t i = 0; i < o->widgets_size; i++) {
        widget_keyboard(o->widgets[i], d);
    }
}

static void _toggle_widget() {
    o->widgets[0]->active = !o->widgets[0]->active;
    widget_on_toggle(o->widgets[0]);
    if (o->widgets[0]->active) {
        wayland_commit(o->widgets[0]->surface);
        wayland_roundtrip();
        printf("Toggle drun on.\n");
    } else {
        o->keyboard->repeating = false;
        wl_surface_commit(o->widgets[0]->surface);
        wayland_roundtrip();
        wayland_hide_surface(o->widgets[0]->surface);
        printf("Toggle drun off.\n");
    }
    wayland_flush();
}

void _display_callback(int fd, void* arg) {
    (void)fd; (void)arg;
    wayland_prepare_display();
    wayland_display_events();
}

void _step_callback(int fd, void* arg) {
    (void)arg;
    uint64_t exp;
    read(fd, &exp, sizeof(exp));
    for (size_t i = 0; i < o->widgets_size; i++) {
        widget_step(o->widgets[i]);
        if (!widget_draw(o->widgets[i])) {
            wayland_flush();
        }
    }
}

void _repeat_callback(int fd, void* arg) {
    (void)arg;
    uint64_t exp;
    read(fd, &exp, sizeof(exp));
    keyboard_repeat_key(o->keyboard);
}

void _ipc_callback(int fd, void* arg) {
    (void)arg;
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
    o->widgets_size = 2;
    o->widgets = calloc(o->widgets_size, sizeof(Widget));

    int anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
             ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
             ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;

    /*
    DRunData* data = malloc(sizeof(DRunData));
    WidgetOps* drun_ops = drun();
    drun_ops->toggle = _toggle_widget;
    o->widgets[0] = widget_create(180, 1440, (void*)data, wayland_shm(), drun_ops, wayland_create_surface(180, 1440, anchor, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY));
    */

    BarData* bdata = malloc(sizeof(BarData));
    WidgetOps* bar_ops = bar();
    bar_ops->toggle = NULL;
    anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
    o->widgets[0] = widget_create(2560, 60, (void*)bdata, wayland_shm(), bar_ops, wayland_create_surface(2560, 60, anchor, ZWLR_LAYER_SHELL_V1_LAYER_TOP));

    WidgetOps* wp_ops = wp();
    bar_ops->toggle = NULL;
    anchor = 0;
    o->widgets[1] = widget_create(2560, 1440, NULL, wayland_shm(), wp_ops, wayland_create_surface(2560, 1440, anchor, ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND));


    // Create event loop
    o->loop = loop_create();
    loop_add_fd(o->loop, sock, _ipc_callback, NULL);
    loop_add_fd(o->loop, wayland_display_fd(), _display_callback, NULL);
    loop_add_fd(o->loop, o->keyboard->repeat_fd, _repeat_callback, NULL);
    loop_add_timer(o->loop, 17, _step_callback);

    wayland_commit_surfaces();

    while (wayland_surfaces_loaded() < (int)o->widgets_size) {
        if (wayland_dispatch() == -1) {
            perror("Wayland dispatch failed during init");
            return 1;
        }
    }

    for (size_t i = 0; i < o->widgets_size; i++) {
        widget_init(o->widgets[i]);
        widget_init_draw(o->widgets[i]);
        WidgetFD* fds = widget_get_fd(o->widgets[i]);
        if (fds == NULL) continue;
        for (int j = 0; j < fds->size; j++) {
            loop_add_fd(o->loop, fds->fd[j], fds->callback[j], (void*)o->widgets[i]->data);
        }
    }
    wayland_flush();

    while (o->active) {
        loop_run(o->loop);
    }
    return 0;
}
