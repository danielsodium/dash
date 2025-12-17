#include "module.h"

void module_set_field(Module* m, int property, int value) {
    m->fields[property] = value;
}

void module_set_dim(Module* m, int x, int y, int w, int h) {
    module_set_field(m, MODULE_X, x);
    module_set_field(m, MODULE_Y, y);
    module_set_field(m, MODULE_W, w);
    module_set_field(m, MODULE_H, h);

    module_set_field(m, MODULE_TARGET_X, x);
    module_set_field(m, MODULE_TARGET_Y, y);
    module_set_field(m, MODULE_TARGET_W, w);
    module_set_field(m, MODULE_TARGET_H, h);
}

void module_set_radi(Module* m, int r1, int r2, int r3, int r4) {
    module_set_field(m, MODULE_R1, r1);
    module_set_field(m, MODULE_R2, r2);
    module_set_field(m, MODULE_R3, r3);
    module_set_field(m, MODULE_R4, r4);

    module_set_field(m, MODULE_TARGET_R1, r1);
    module_set_field(m, MODULE_TARGET_R2, r2);
    module_set_field(m, MODULE_TARGET_R3, r3);
    module_set_field(m, MODULE_TARGET_R4, r4);
}

