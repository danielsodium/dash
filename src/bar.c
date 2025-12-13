#include "bar.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>

#include "status.h"

void system_widget(char* s) {
    s = print_cpu(s);
    *(s++) = '\n';
    s = print_mem(s);
    *(s++) = '\n';
    s = print_battery(s);
    *(s-1) = '\0';
}

void clock_widget(char* s) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buffer[32];
    char date_buffer[32];
    strftime(time_buffer, sizeof(time_buffer), "%I:%M %p", tm_info);
    strftime(date_buffer, sizeof(date_buffer), "%A", tm_info);
    sprintf(s, "%12s\n%12s", time_buffer, date_buffer);
}

void bar_init(cairo_t* cairo, void* data) {
    BarData* d = data;
    d->layout = pango_cairo_create_layout(cairo);
    d->font = pango_font_description_from_string("JetBrainsMono Nerd Font 18");
    pango_layout_set_font_description(d->layout, d->font);
    d->top_str[0] = '\0';
    d->bot_str[0] = '\0';
    d->frames = 100;
}

int bar_step(int* active, void* data) {
    (void)active;
    BarData* d = data;
    if (d->frames < 60) {
        d->frames++;
        return 0;
    }
    d->frames = 0;
    clock_widget(d->top_str);
    system_widget(d->bot_str);
    return 1;
}

int bar_draw(cairo_t* cairo, void* data) {
    BarData* d = data;
    int text_width, text_height;

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
    return 0;
}

void bar_destroy(void* data) {
    BarData* d = data;
    g_object_unref(d->layout);
    pango_font_description_free(d->font);
    free(d);
}

WidgetOps* bar() {
    WidgetOps* b = malloc(sizeof(WidgetOps));
    *b = (WidgetOps) {
        .init = bar_init,
        .draw = bar_draw,
        .keyboard = NULL,
        .step = bar_step,
        .destroy = bar_destroy,
        .on_toggle = NULL
    };
    return b;
}
