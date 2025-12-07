#include "widget.h"

void widget_init(Widget* w) {
    if (!w->active || !w->operators->init) return;
    w->operators->init(w->canvas->cairo, w->data);
}

void widget_step(Widget* w) {
    if (!w->active || !w->operators->step) return;
    int widget_active = 1;
    w->operators->step(&widget_active, w->data);
    if (!widget_active) w->operators->toggle();
}

int widget_draw(Widget* w) {
    if (!w->active) return 1;
    if (!w->operators->draw) return 1;
    if (w->operators->draw(w->canvas->cairo, w->data)) return 1;
    struct wl_buffer* buffer = canvas_get_current_buffer(w->canvas);

    wl_surface_attach(w->surface, buffer,0,0);
    wl_surface_damage_buffer(w->surface, 0, 0, w->width, w->height);
    wl_surface_commit(w->surface);
    canvas_swap_buffers(w->canvas);
    return 0;
}

void widget_keyboard(Widget* w, KeyboardData* d) {
    if (!w->active || !w->operators->keyboard) return;
    int widget_active = 1;
    w->operators->keyboard(d, &widget_active, w->data);
    if (!widget_active) w->operators->toggle();
}

void widget_destroy(Widget* w) {
    if (!w->active || !w->operators->destroy) return;
    w->operators->destroy(w->data);
}

Widget* widget_create(size_t width, size_t height, void* data, struct wl_shm* shm, WidgetOps* operators, struct wl_surface* surface) {
    Widget* w = calloc(1, sizeof(Widget));
    *w = (Widget) {
        .active = 1,
        .width = width,
        .height = height,
        .data = data,
        .canvas = canvas_create(width, height, shm),
        .operators = operators,
        .surface = surface
    };
    return w;
}

