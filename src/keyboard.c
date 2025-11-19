#include "keyboard.h"

#include <stdlib.h>

static void _handle_keyboard(void *data, struct wl_seat *seat, uint32_t caps) {
    (void)seat;
    Keyboard* k = data;
    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !k->inst) {
        k->inst = wl_seat_get_keyboard(k->seat);
        wl_keyboard_add_listener(k->inst, k->listener, k);
    }
}

static void _seat_name(void *data, struct wl_seat *seat, const char *name) {
    (void)data; (void)seat; (void)name;
}

Keyboard* keyboard_create(struct wl_seat* seat) {
    Keyboard* k = malloc(sizeof(Keyboard));

    k->inst = NULL;
    k->context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    k->seat = seat;
    k->seat_listener = malloc(sizeof(struct wl_seat_listener));
    *(k->seat_listener) = (struct wl_seat_listener){
        .capabilities = _handle_keyboard, .name = _seat_name
    };
    wl_seat_add_listener(k->seat, k->seat_listener, k);

    k->listener = malloc(sizeof(struct wl_keyboard_listener));
    *(k->listener) = (struct wl_keyboard_listener){
        .keymap = on_keymap,
        .leave =  on_leave,
        .enter =  on_enter,
        .key = on_key,
        .modifiers = on_mods,
        .repeat_info = on_repeat
    };

    return k;
}



void keyboard_destroy(Keyboard* k) {
    xkb_state_unref(k->state);
    xkb_keymap_unref(k->keymap);
    xkb_context_unref(k->context);
    wl_keyboard_destroy(k->inst);
    free(k->listener);
    free(k->seat_listener);
}

void on_key(void *data, struct wl_keyboard *kbd, uint32_t serial,
                    uint32_t time, uint32_t key, uint32_t state) {}
void on_keymap(void *d, struct wl_keyboard *k, uint32_t f, int32_t fd, 
                       uint32_t s) {}
void on_enter(void *d, struct wl_keyboard *k, uint32_t s, 
                      struct wl_surface *surf, struct wl_array *keys) {}
void on_leave(void *d, struct wl_keyboard *k, uint32_t s, 
                      struct wl_surface *surf) {}
void on_mods(void *d, struct wl_keyboard *k, uint32_t s, uint32_t dep, 
                     uint32_t lat, uint32_t lock, uint32_t grp) {}
void on_repeat(void *d, struct wl_keyboard *k, int32_t rate, int32_t delay) {}
void handle_keyboard(void *data, struct wl_seat *seat, uint32_t caps){}
