#ifndef _DRUN_H_
#define _DRUN_H_

#include <cairo/cairo.h>
#include <xkbcommon/xkbcommon.h>
#include <pango/pangocairo.h>

#include "widget.h"
#include "keyboard.h"

#define WIDTH_CHARS 12

typedef struct {

    int update;

    PangoLayout* layout;
    PangoFontDescription* font;

    char input[1024];
    char** bins;
    size_t bins_size;
    size_t bins_max;

    char options[5][32];
    size_t options_size;

} DRunData;

void drun_init(cairo_t* cairo, void* data);
int drun_draw(cairo_t* cairo, void* data);
int drun_on_key(KeyboardData* event_data, int* active, void* data);
void drun_destroy(void* data);

WidgetOps* drun();

#endif
