#include "util.h"
#include <math.h>

int lerp(float a, float b, float p) {
    float ans = a * (1.0 - p) + (b * p);
    return (a - b < 0) ? ceil(ans) : floor(ans);
}

void cairo_rectangle_radius(cairo_t* cr, int x, int y, int w, int h, int r1, int r2, int r3, int r4) {
    if (r1 != 0) {
        cairo_move_to(cr, x, y + r1);
        cairo_arc(cr, x + r1, y + r1, r1, M_PI, 3 * M_PI / 2);
    } else {
        cairo_move_to(cr, x, y);
    }

    if (r2 != 0) {
        cairo_arc(cr, x + w - r2, y + r2, r2, -M_PI / 2, 0);
    } else {
        cairo_line_to(cr, x + w, y);
    }

    if (r3 != 0) {
        cairo_arc(cr, x + w - r3, y + h - r3, r3, 0, M_PI / 2);
    } else {
        cairo_line_to(cr, x + w, y + h);
    }

    if (r4 != 0) {
        cairo_arc(cr, x + r4, y + h - r4, r4, M_PI / 2, M_PI);
    } else {
        cairo_line_to(cr, x, y + h);
    }

    cairo_close_path(cr);
}
