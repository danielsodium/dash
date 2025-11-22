#ifndef _WIDGET_H_
#define _WIDGET_H_

#include "canvas.h"
#include "common.h"

struct Widget {
    int active;
    size_t width, height;

    int init_attached;
    void (*init)(cairo_t*, void*);

    int draw_attached;
    int (*draw)(cairo_t*, int*, void*);
    Canvas* canvas;
    int buffer_attached;

    int step_attached;
    int (*step)(int*, void*);

    int keyboard_attached;
    int (*keyboard)(KeyboardData*, int*, void*);

    int destroy_attached;
    void (*destroy)(void*);

    int data_attached;
    void* data;

    int surface_attached;
    struct wl_surface* surface;

    void(*toggle)(void);
};

Widget* widget_create(size_t width, size_t height, struct wl_shm* shm, void(*toggle)(void));

void widget_attach_data(Widget* w, void* data);
void widget_attach_destroy(Widget* w, void(*destroy)(void*));
void widget_attach_draw(Widget* w, int(*draw)(cairo_t*, int*, void*));
void widget_attach_surface(Widget* w, struct wl_surface* surface);

void widget_attach_init(Widget* w, void(*init)(cairo_t*, void*));
void widget_attach_step(Widget* w, int(*step)(int*, void*));
void widget_attach_keyboard(Widget* w, int(*keyboard)(KeyboardData*, int*, void*));

void widget_init(Widget* w);
void widget_step(Widget* w);
void widget_destroy(Widget* w);
void widget_keyboard(Widget* w, KeyboardData* d);
int widget_draw(Widget* w);

#endif
