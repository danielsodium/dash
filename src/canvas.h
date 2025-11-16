#ifndef _CANVAS_H_
#define _CANVAS_H_

#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

typedef struct {
    cairo_surface_t* surface;
    struct wl_buffer* buffer;
    cairo_t* cairo;
} Canvas;

Canvas* canvas_create(int width, int height, struct wl_shm* shm);
void canvas_start_buffer(Canvas* c);
void canvas_draw_buffer(Canvas* c);
void canvas_destroy(Canvas* c);

#endif
