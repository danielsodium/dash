#include "keyboard.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

static void _on_key(void *data, struct wl_keyboard *kbd, uint32_t serial,
                       uint32_t time, uint32_t key, uint32_t state) {
    (void)data; (void)kbd; (void)serial; (void)time;
    Keyboard* k = data;
    if (state != WL_KEYBOARD_KEY_STATE_PRESSED)
        return;

    xkb_keysym_t sym = xkb_state_key_get_one_sym(k->state, key+8);
    k->on_key(k->w, &sym);
}

static void _on_keymap(void *data, struct wl_keyboard *inst, uint32_t f, int32_t fd, uint32_t s) { 
    (void)inst;
    char* map_str;
    Keyboard* k = data;

    if (f != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd); 
        return;
    }
    map_str = mmap(NULL, s, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    if (map_str == MAP_FAILED) return;

    k->keymap = xkb_keymap_new_from_string(k->context,
                                                    map_str,
                                                    XKB_KEYMAP_FORMAT_TEXT_V1,
                                                    XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(map_str, s);
    if (!k->keymap) return;
    k->state = xkb_state_new(k->keymap);
}

static void _on_enter(void *d, struct wl_keyboard *k, uint32_t s, struct wl_surface *surf, struct wl_array *keys) {
    (void)d; (void)k; (void)s; (void)surf; (void)keys;
}

static void _on_leave(void *d, struct wl_keyboard *k, uint32_t s, struct wl_surface *surf) {
    (void)d; (void)k; (void)s; (void)surf;
}

static void _on_mods(void *d, struct wl_keyboard *k, uint32_t s, uint32_t dep, uint32_t lat, uint32_t lock, uint32_t grp) {
    (void)d; (void)k; (void)s; (void)dep; (void)lat; (void)lock; (void)grp;
}
static void _on_repeat(void *d, struct wl_keyboard *k, int32_t rate, int32_t delay) {
    (void)d; (void)k; (void)rate; (void)delay;
}

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

Keyboard* keyboard_create(Window* w, struct wl_seat* seat) {
    Keyboard* k = malloc(sizeof(Keyboard));

    k->inst = NULL;
    k->w = w;
    k->context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    k->seat = seat;
    k->seat_listener = malloc(sizeof(struct wl_seat_listener));
    *(k->seat_listener) = (struct wl_seat_listener){
        .capabilities = _handle_keyboard, .name = _seat_name
    };
    wl_seat_add_listener(k->seat, k->seat_listener, k);

    k->listener = malloc(sizeof(struct wl_keyboard_listener));
    *(k->listener) = (struct wl_keyboard_listener){
        .keymap = _on_keymap,
        .leave =  _on_leave,
        .enter =  _on_enter,
        .key = _on_key,
        .modifiers = _on_mods,
        .repeat_info = _on_repeat
    };

    return k;
}

void keyboard_attach_on_key(Keyboard* k, void(*on_key)(Window*, xkb_keysym_t*)) {
    k->on_key = on_key;
}

void keyboard_destroy(Keyboard* k) {
    xkb_state_unref(k->state);
    xkb_keymap_unref(k->keymap);
    xkb_context_unref(k->context);
    wl_keyboard_destroy(k->inst);
    free(k->listener);
    free(k->seat_listener);
}

