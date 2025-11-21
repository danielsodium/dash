#ifndef _WIDGET_H_
#define _WIDGET_H_

struct Widget {
    int active;
    int width, height;

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

#endif
