#ifndef _DRAW_H_
#define _DRAW_H_

#include <cairo/cairo.h>
#include <wayland-client.h>

#include "window.h"

typedef struct {
    cairo_surface_t* surface;
    struct wl_buffer* buffer;
} Canvas;

Canvas* draw_init(Window* w);
void draw_bar(Canvas* c);

#endif
