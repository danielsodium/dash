#include "wallpaper.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>

void wp_init(cairo_t* cairo, void* data) {
    (void) data;
    cairo_set_source_rgb(cairo, 0.1, 0.1, 0.1);
    cairo_paint(cairo);

    cairo_surface_t *image =
        cairo_image_surface_create_from_png("/home/dan/Downloads/mountains.png");

    cairo_set_source_surface(cairo, image, 0, 0);
    cairo_paint(cairo);
}

WidgetOps* wp() {
    WidgetOps* w = malloc(sizeof(WidgetOps));
    *w = (WidgetOps) {
        .init = wp_init,
        .draw = NULL,
        .keyboard = NULL,
        .step = NULL,
        .destroy = NULL,
        .on_toggle = NULL
    };
    return w;
}
