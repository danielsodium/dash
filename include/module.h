#ifndef _MODULE_H_
#define _MODULE_H_

#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include <stddef.h>

typedef enum {
    MODULE_ACTIVATE = 1 << 0,
    MODULE_DEACTIVATE = 1 << 1,
    MODULE_UPDATE = 1 << 2
} ModuleCallbackReturn;

typedef struct Module {
    int active;
    int activate_queued;
    int id;
    int x, y, w, h;
    int r1, r2, r3, r4;

    int* fds;
    size_t fds_size;
    void* data;

    void(*init)(struct Module*);
    void(*draw)(struct Module*, cairo_t*, PangoLayout*);
    int (*callback)(struct Module*, int, PangoLayout*);
} Module;

typedef struct {
    int x, y;
    char song[128];
} PlayerctlData;

typedef struct {
    int x, y;
    char time[32];
} ClockData;

void module_set_dim(Module* m, int x, int y, int w, int h);
void module_set_radi(Module* m, int r1, int r2, int r3, int r4);

void playerctl_module(Module* m);
void clock_module(Module* m);

#endif
