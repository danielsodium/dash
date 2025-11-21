#ifndef _WIDGET_H_
#define _WIDGET_H_

#include "canvas.h"

struct Widget {
    int active;
    size_t width, height;

    int init_attached;
    void (*init)(cairo_t*, void*);

    int draw_attached;
    void (*draw)(cairo_t*, int*, void*);
    Canvas* canvas;

    int step_attached;
    int (*step)(int*, void*);

    int keyboard_attached;
    int (*on_keyboard)(KeyboardData*, int*, void*);

    int destroy_attached;
    void (*destroy)(void*);

    void* data;
};

Widget* widget_create(size_t width, size_t height, int anchor, int layer);

void widget_attach_data(Widget* w, void* data);
void widget_attach_destroy(Widget* w, void(*destroy)(void*));

int widget_attach_init(Widget* w, void(*init)(cairo*, void*));
int widget_attach_step(Widget* w, int(*step)(int*, void*));
void window_attach_keyboard_listener(Widget* w, int(*on_keyboard)(KeyboardData*, int*, void*));

#endif
