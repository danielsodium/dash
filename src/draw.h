#ifndef _DRAW_H_
#define _DRAW_H_

#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "draw.h"

typedef enum {
    ANCHOR_TOP,
    ANCHOR_BOTTOM,
    ANCHOR_CENTER
} Anchor;

typedef struct {
    cairo_surface_t* surface;
    struct wl_buffer* buffer;
    cairo_t* cairo;

    PangoFontDescription* font;
    PangoLayout* layout;
} Canvas;

Canvas* canvas_init(struct wl_shm* shm);
void draw_start(Canvas* c);
void draw_string(Canvas* c, char* str, Anchor a);
void draw_to_buffer(Canvas* c);
void canvas_destroy(Canvas* c);

#endif
