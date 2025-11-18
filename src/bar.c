#include "bar.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>

#include "widgets.h"

void bar_init(BarData* d, cairo_t* cairo) {
    d->layout = pango_cairo_create_layout(cairo);
    d->font = pango_font_description_from_string("JetBrainsMono Nerd Font 18");
    pango_layout_set_font_description(d->layout, d->font);

    d->update = 1;
}

void bar_step(int* active, void* data) {
    (void)active;
    BarData* d = data;
    clock_widget(d->top_str);
    system_widget(d->bot_str);
    d->update = 1;
}

int bar_draw(cairo_t* cairo, int* active, void* data) {
    (void)active;
    BarData* d = data;
    int text_width, text_height;

    if (!d->update) return 0;

    cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 1.0);
    cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cairo);

    cairo_move_to(cairo, 0, 0);
    cairo_set_source_rgba(cairo, 1.0,1.0,1.0,1.0);
    pango_layout_set_text(d->layout, d->top_str, -1);
    pango_cairo_update_layout(cairo, d->layout);
    pango_cairo_show_layout(cairo, d->layout);

    pango_layout_set_text(d->layout, d->bot_str, -1);
    pango_layout_get_pixel_size(d->layout, &text_width, &text_height);
    cairo_move_to(cairo, 10, (1440 - text_height - 10));
    pango_cairo_show_layout(cairo, d->layout);

    d->update = 0;
    return 1;
}

void bar_destroy(BarData* d) {
    g_object_unref(d->layout);
    pango_font_description_free(d->font);
    free(d);
}
