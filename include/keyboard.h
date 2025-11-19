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
};


Keyboard* keyboard_create(struct wl_seat* seat);

void keyboard_attach_on_key(Keyboard* k, int(*on_key)(xkb_keysym_t*, int*, void*));

// --------------------------------------------------------- //
//
void on_key(void *data, struct wl_keyboard *kbd, uint32_t serial,
                    uint32_t time, uint32_t key, uint32_t state);
void on_keymap(void *d, struct wl_keyboard *k, uint32_t f, int32_t fd, 
                       uint32_t s);
void on_enter(void *d, struct wl_keyboard *k, uint32_t s, 
                      struct wl_surface *surf, struct wl_array *keys);
void on_leave(void *d, struct wl_keyboard *k, uint32_t s, 
                      struct wl_surface *surf);
void on_mods(void *d, struct wl_keyboard *k, uint32_t s, uint32_t dep, 
                     uint32_t lat, uint32_t lock, uint32_t grp);
void on_repeat(void *d, struct wl_keyboard *k, int32_t rate, int32_t delay);

void handle_keyboard(void *data, struct wl_seat *seat, uint32_t caps);

#endif
