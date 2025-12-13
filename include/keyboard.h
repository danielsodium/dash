#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "common.h"

struct Keyboard {
    struct wl_keyboard* inst;
    struct wl_keyboard_listener* listener;
    struct xkb_keymap* keymap;
    struct xkb_state* state;
    struct xkb_context* context;

    struct wl_seat* seat;
    struct wl_seat_listener* seat_listener;

    // Repeat keys when pressed
    int rate, delay;
    int repeat_fd;
    xkb_keysym_t last_pressed;
    int repeating;

    void(*on_event)(KeyboardData*);
    void(*toggle)(void);
};

Keyboard* keyboard_attach(struct wl_seat* seat, 
                          void(*on_event)(KeyboardData*));

void keyboard_destroy(Keyboard* k);

void keyboard_repeat_key(Keyboard* k);

#endif
