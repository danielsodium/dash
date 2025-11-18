#ifndef _DRUN_H_
#define _DRUN_H_

#include <cairo/cairo.h>
#include <xkbcommon/xkbcommon.h>
#include <pango/pangocairo.h>

#define WIDTH_CHARS 12

typedef struct {
    PangoLayout* layout;
    PangoFontDescription* font;

    char input[1024];
    char** bins;
    size_t bins_size;
    size_t bins_max;

    char options[5][32];
    size_t options_size;

} DRunData;

void drun_init(DRunData* d, cairo_t* cairo);
void drun_draw(cairo_t* cairo, int* active, void* data);
void drun_on_key(xkb_keysym_t* key, int* active, void* data);
void drun_destroy(DRunData* d);

#endif
