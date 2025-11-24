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
    int widget_active = 1;
    w->step(&widget_active, w->data);
    if (!widget_active) w->toggle();
}

ATTACH(draw, int(*draw)(cairo_t*, void*));
int widget_draw(Widget* w) {
    if (!w->active) return 1;
    if (!w->surface_attached || !w->draw_attached) return 1;
    if (w->draw(w->canvas->cairo, w->data)) return 1;
    struct wl_buffer* buffer = canvas_get_current_buffer(w->canvas);

    wl_surface_attach(w->surface, buffer,0,0);
    wl_surface_damage(w->surface, 0, 0, w->width, w->height);
    wl_surface_commit(w->surface);
    canvas_swap_buffers(w->canvas);
    return 0;
}

ATTACH(keyboard, int(*keyboard)(KeyboardData*, int*, void*));
void widget_keyboard(Widget* w, KeyboardData* d) {
    if (!w->active || !w->keyboard_attached) return;
    int widget_active = 1;
    w->keyboard(d, &widget_active, w->data);
    if (!widget_active) w->toggle();
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
    *w = (Widget) {
        .active = 1,
        .width = width,
        .height = height,
        .canvas = canvas_create(width, height, shm),
        .toggle = toggle
    };
    return w;
}
