#ifndef _CANVAS_H_
#define _CANVAS_H_

#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

typedef struct {
    void* data;
    size_t data_size;
    struct wl_buffer* buffers[2];  // Changed from single buffer
    cairo_surface_t* surfaces[2];   // Two surfaces
    cairo_t* cairo;
    int current_buffer;
    int width, height, stride;
} Canvas;

Canvas* canvas_create(int width, int height, struct wl_shm* shm);
void canvas_swap_buffers(Canvas* c);
struct wl_buffer* canvas_get_current_buffer(Canvas* c);
void canvas_destroy(Canvas* c);

#endif
