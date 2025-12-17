#ifndef _MODULE_H_
#define _MODULE_H_

#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include <stddef.h>

typedef enum {
    MODULE_ACTIVE = 0,
    MODULE_ID,
    MODULE_X,
    MODULE_Y,
    MODULE_W,
    MODULE_H,
    MODULE_TARGET_X,
    MODULE_TARGET_Y,
    MODULE_TARGET_W,
    MODULE_TARGET_H,
    MODULE_R1,
    MODULE_R2,
    MODULE_R3,
    MODULE_R4,
    MODULE_TARGET_R1,
    MODULE_TARGET_R2,
    MODULE_TARGET_R3,
    MODULE_TARGET_R4
} ModuleFields;

typedef enum {
    MODULE_ACTIVATE = 1 << 0,
    MODULE_DEACTIVATE = 1 << 1,
    MODULE_UPDATE = 1 << 2
} ModuleCallbackReturn;

typedef struct Module {
    int fields[17];

    int* fds;
    size_t fds_size;
    void* data;

    void(*init)(struct Module* m);
    void(*draw)(struct Module* m, cairo_t*, PangoLayout* layout);
    int (*callback)(struct Module* m, int);
} Module;

typedef struct {
    char song[128];
} PlayerctlData;

void module_set_field(Module* m, int field, int value);
void module_set_dim(Module* m, int x, int y, int w, int h);
void module_set_radi(Module* m, int r1, int r2, int r3, int r4);

void playerctl_module(Module* m);

#endif
