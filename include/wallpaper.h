#ifndef _WALLPAPER_H_
#define _WALLPAPER_H_

#include <cairo/cairo.h>
#include <xkbcommon/xkbcommon.h>
#include <pango/pangocairo.h>

#include "widget.h"

void wp_init(cairo_t* cairo, void* data);
int wp_draw(cairo_t* cairo, void* data);

WidgetOps* wp ();

#endif
