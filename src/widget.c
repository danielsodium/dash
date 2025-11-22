#include "widget.h"

#define ATTACH(event, callback)\
    void widget_attach_##event(Widget* w, callback) {\
        w->event##_attached = 1;\
        w->event = event;\
    }

ATTACH(init, void(*init)(cairo_t*, void*));
void widget_init(Widget* w) {
    if (!w->active || !w->init_attached) return;
    w->init(w->canvas->cairo, w->data);
}

ATTACH(step, int(*step)(int*, void*));
void widget_step(Widget* w) {
    if (!w->active || !w->step_attached) return;
    w->step(&w->active, w->data);
}

ATTACH(draw, int(*draw)(cairo_t*, int*, void*));
int widget_draw(Widget* w) {
    if (!w->active) {
        if (w->buffer_attached) {
            w->buffer_attached = 0;
            wl_surface_attach(w->surface, NULL,0,0);
            wl_surface_damage(w->surface, 0, 0, w->width, w->height);
            wl_surface_commit(w->surface);
            return 0;
        }
        return 1;
    }
    if (!w->surface_attached || !w->draw_attached) return 1;
    if (w->draw(w->canvas->cairo, &w->active, w->data)) return 1;
    wl_surface_attach(w->surface, w->canvas->buffer,0,0);
    wl_surface_damage(w->surface, 0, 0, w->width, w->height);
    wl_surface_commit(w->surface);

    w->buffer_attached = 1;
    return 0;
}

ATTACH(keyboard, int(*keyboard)(KeyboardData*, int*, void*));
void widget_keyboard(Widget* w, KeyboardData* d) {
    if (!w->active || !w->keyboard_attached) return;
    w->keyboard(d, &w->active, w->data);
}

ATTACH(destroy, void(*destroy)(void*));
void widget_destroy(Widget* w) {
    if (!w->active || !w->destroy_attached) return;
    w->destroy(w->data);
}

ATTACH(surface, struct wl_surface* surface);
ATTACH(data, void* data);

Widget* widget_create(size_t width, size_t height, struct wl_shm* shm, void(*toggle)(void)) {
    Widget* w = calloc(1, sizeof(Widget));
    w->width = width;
    w->height = height;
    w->canvas = canvas_create(width, height, shm);
    w->buffer_attached = 0;
    w->active = 1;
    w->data = NULL;
    w->toggle = toggle;
    return w;
}
