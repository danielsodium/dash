#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "window.h"
#include "drun.h"
#include "bar.h"

void run_bar() {
    Window* w;
    BarData data;
    int layer, anchor;

    layer = ZWLR_LAYER_SHELL_V1_LAYER_TOP;
    anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
             ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
             ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;

    w = window_create(180, 1440, anchor, layer);
    window_commit(w, bar_draw);

    bar_init(&data, w->canvas->cairo);
    window_set_data(w, (void*) &data);
    window_loop(w);
    bar_destroy(&data);
    window_destroy(w);
}

void run_drun() {
    Window* w;
    DRunData data;
    int layer, anchor;

    layer = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
    anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
             ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
             ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
    w = window_create(180, 1440, anchor, layer);
    window_attach_keyboard_listener(w, drun_on_key);

    window_commit(w, drun_draw);

    drun_init(&data, w->canvas->cairo);
    window_set_data(w, (void*) &data);
    window_loop(w);
    drun_destroy(&data);
    window_destroy(w);
}

int main() {
#ifdef DRUN
    run_drun();
#else
    run_bar();
#endif
    return 0;
}
