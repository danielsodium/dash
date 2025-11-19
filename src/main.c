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
    BarData* data = malloc(sizeof(BarData));
    int layer, anchor;

    layer = ZWLR_LAYER_SHELL_V1_LAYER_TOP;
    anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
             ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
             ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;

    w = window_create(180, 1440, anchor, layer);
    window_attach_init(w, bar_init);
    window_attach_step(w, 1000, bar_step);
    window_attach_draw(w, bar_draw);
    window_attach_data(w, (void*) data);
    window_attach_destroy(w, bar_destroy);

    window_run(w);
}

void run_drun() {
    Window* w;
    DRunData* data = malloc(sizeof(DRunData));
    int layer, anchor;

    layer = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
    anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
             ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
             ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;

    w = window_create(180, 1440, anchor, layer);
    window_attach_draw(w, drun_draw);
    window_attach_init(w, drun_init);
    window_attach_data(w, (void*) data);
    window_attach_destroy(w, drun_destroy);
    window_attach_keyboard_listener(w, drun_on_key);
    window_run(w);
}

int main() {
#ifdef DRUN
    run_drun();
#else
    run_bar();
#endif
    return 0;
}
