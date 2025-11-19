#ifndef _DRUN_H_
#define _DRUN_H_

#include <cairo/cairo.h>
#include <xkbcommon/xkbcommon.h>
#include <pango/pangocairo.h>

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
void drun_draw(cairo_t* cairo, int* active, void* data);
int drun_on_key(xkb_keysym_t* key, int* active, void* data);
void drun_destroy(void* data);

#endif
