#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "window.h"

void draw(cairo_t* cairo, int* active, void* data) {

}

void on_key(xkb_keysym_t* key, int* active, void* data) {

}

int main() {
    Window* w;
    int layer, anchor;

    layer = ZWLR_LAYER_SHELL_V1_LAYER_TOP;
    anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
             ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
             ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
    w = window_create(100, 100, anchor, layer);
    window_attach_keyboard_listener(w, on_key);
    window_commit(w, draw);
    window_loop(w);
    window_destroy(w);
    return 0;
}
