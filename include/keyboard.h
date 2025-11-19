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

    void(*on_key)(Window*, xkb_keysym_t*);

    Window* w;
};


Keyboard* keyboard_create(Window* w, struct wl_seat* seat);
void keyboard_attach_on_key(Keyboard* k, void(*on_key)(Window*, xkb_keysym_t*));

#endif
