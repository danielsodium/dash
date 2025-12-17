#ifndef _UTIL_H_
#define _UTIL_H_

#include <cairo/cairo.h>

int lerp(float a, float b, float p);
void cairo_rectangle_radius(cairo_t* cr, int x, int y, int w, int h, int r1, int r2, int r3, int r4);

#endif
