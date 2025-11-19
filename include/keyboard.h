#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "common.h"

enum KeyboardEvent {
    KEY,
    KEYMAP,
    LEAVE,
    ENTER,
    MODIFIER,
    REPEAT
};

struct KeyboardData {
    enum KeyboardEvent event;
    xkb_keysym_t* key;
};

struct Keyboard {
    struct wl_keyboard* inst;
    struct wl_keyboard_listener* listener;
    struct xkb_keymap* keymap;
    struct xkb_state* state;
    struct xkb_context* context;

    struct wl_seat* seat;
    struct wl_seat_listener* seat_listener;

    Window* w;

    void(*on_event)(Window*, KeyboardData*);
};


Keyboard* keyboard_attach(Window* w, 
                          struct wl_seat* seat, 
                          void(*on_event)(Window*, KeyboardData*));

void keyboard_destroy(Keyboard* k);

#endif
