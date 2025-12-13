#ifndef _BAR_H_
#define _BAR_H_

#include <cairo/cairo.h>
#include <xkbcommon/xkbcommon.h>
#include <pango/pangocairo.h>

#include "widget.h"

#define WIDTH_CHARS 12

typedef struct {

    int update;
    int frames;

    PangoLayout* layout;
    PangoFontDescription* font;

    char top_str[512];
    char bot_str[512];

} BarData;

void bar_init(cairo_t* cairo, void* data);
int bar_step(int* active, void* data);
int bar_draw(cairo_t* cairo, void* data);
void bar_destroy(void* data);

WidgetOps* bar();

#endif
