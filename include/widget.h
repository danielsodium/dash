#ifndef _WIDGET_H_
#define _WIDGET_H_

#include "canvas.h"
#include "common.h"

struct WidgetOps {
    void (*init)(cairo_t*, void*);
    int (*draw)(cairo_t*, void*);
    int (*step)(int*, void*);
    int (*keyboard)(KeyboardData*, int*, void*);
    void (*destroy)(void*);
    void(*toggle)(void);
    void(*on_toggle)(int, void*);
};

struct Widget {
    int active;
    size_t width, height;
    void* data;
    struct wl_surface* surface;
    WidgetOps* operators;
    Canvas* canvas;
};

Widget* widget_create(size_t width, size_t height, void* data, struct wl_shm* shm, WidgetOps* operators, struct wl_surface* surface);

void widget_init(Widget* w);
void widget_step(Widget* w);
void widget_destroy(Widget* w);
void widget_keyboard(Widget* w, KeyboardData* d);
void widget_on_toggle(Widget* w);
int widget_draw(Widget* w);
void widget_init_draw(Widget* w);


#endif
