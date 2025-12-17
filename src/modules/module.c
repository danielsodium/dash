#include "module.h"

void module_set_dim(Module* m, int x, int y, int w, int h) {
    m->x = x;
    m->y = y;
    m->w = w;
    m->h = h;
}

void module_set_radi(Module* m, int r1, int r2, int r3, int r4) {
    m->r1 = r1;
    m->r2 = r2;
    m->r3 = r3;
    m->r4 = r4;
}

