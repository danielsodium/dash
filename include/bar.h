#ifndef _BAR_H_
#define _BAR_H_

#include <cairo/cairo.h>
#include <xkbcommon/xkbcommon.h>
#include <pango/pangocairo.h>

#define WIDTH_CHARS 12

typedef struct {

    int update;

    PangoLayout* layout;
    PangoFontDescription* font;

    char top_str[512];
    char bot_str[512];

} BarData;

void bar_init(cairo_t* cairo, void* data);
int bar_step(int* active, void* data);
void bar_draw(cairo_t* cairo, int* active, void* data);
void bar_destroy(void* data);

#endif
