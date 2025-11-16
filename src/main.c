#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "window.h"

int main() {

    Window* w;
    time_t last_update, now;
    int layer, anchor;

    layer = ZWLR_LAYER_SHELL_V1_LAYER_TOP;
    anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
             ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
             ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
    w = window_create(100, 100, anchor, layer);
    window_attach_keyboard_listener(w);
    window_commit(w);
    window_draw(w);

    last_update = 0;
    while (w->active) {
        now = time(NULL);
        if (now != last_update) {
            window_draw(w);
            last_update = now;
        }
        window_handle_events(w);
        usleep(100000);
    }
    window_destroy(w);
    return 0;
}
