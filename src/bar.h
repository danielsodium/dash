#ifndef _BAR_H_
#define _BAR_H_

#include <cairo/cairo.h>
#include <xkbcommon/xkbcommon.h>
#include <pango/pangocairo.h>

#define WIDTH_CHARS 12

typedef struct {
    PangoLayout* layout;
    PangoFontDescription* font;
} BarData;

void bar_init(BarData* d, cairo_t* cairo);
void bar_draw(cairo_t* cairo, int* active, void* data);
void bar_destroy(BarData* d);

#endif
