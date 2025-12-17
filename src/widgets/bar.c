#include "bar.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>

#include "status.h"


void create_animation(BarData* b, int module_num, int wait, int field, int value) {
    AnimationNode* a = malloc(sizeof(AnimationNode));
    if (b->animation_tail)
        b->animation_tail->next = a;
    else
        b->animation_head = a;
    b->animation_tail = a;

    *(a) = (AnimationNode) {
        .module = module_num,
        .field = field,
        .value = value,
        .wait = wait,
        .next = NULL
    };
}

void activate_module(BarData* b, size_t x) {

    int left = -1;
    int right = -1;
    int module_width = b->modules[x].fields[MODULE_W];
    // Find left and right modules
    for (int i = (int)x - 1; i >= 0; i--) {
        if (b->modules[i].fields[MODULE_ACTIVE]) {
            left = i;
            break;
        }
    }
    for (size_t i = x + 1; i < b->modules_size; i++) {
        if (b->modules[i].fields[MODULE_ACTIVE]) {
            right = i;
            break;
        }
    }

    // round the inserting module
    module_set_radi(b->modules + x, 10, 10, 0, 0);

    // round corner on adjacent modules
    if (left != -1) create_animation(b, left, 0, MODULE_TARGET_R2, 10);
    if (right != -1) create_animation(b, right, 0, MODULE_TARGET_R1, 10);

    // move all previous modules
    int first = 1;
    for (size_t i = 0; i < x; i++) {
        create_animation(b, i, first, MODULE_TARGET_X, b->modules[i].fields[MODULE_X] - module_width/2);
        first = 0;
    }
    for (size_t i = x+1; i < b->modules_size; i++) {
        create_animation(b, i, 0, MODULE_TARGET_X, b->modules[i].fields[MODULE_X] + module_width/2);
    }

    if (left == -1 && right == -1) {
        create_animation(b, x, 0, MODULE_X, b->w/2 - module_width/2);
        create_animation(b, x, 0, MODULE_TARGET_X, b->w/2 - module_width/2);
    } else if (right == -1) {
        create_animation(b, x, 0, MODULE_X, b->modules[left].fields[MODULE_X] + b->modules[left].fields[MODULE_W] - module_width/2);
        create_animation(b, x, 0, MODULE_TARGET_X, b->modules[left].fields[MODULE_X] + b->modules[left].fields[MODULE_W] - module_width/2);
    } else {
        create_animation(b, x, 0, MODULE_X, b->modules[right].fields[MODULE_X] - module_width/2);
        create_animation(b, x, 0, MODULE_TARGET_X, b->modules[right].fields[MODULE_X] - module_width/2);
    }

    create_animation(b, x, 1, MODULE_Y, b->h);
    create_animation(b, x, 0, MODULE_TARGET_Y, b->h);
    create_animation(b, x, 1, MODULE_ACTIVE, 1);
    create_animation(b, x, 1, MODULE_TARGET_Y, 0);

    first = 1;
    if (left != -1) create_animation(b, x, first ? !(first = 0) : 0, MODULE_TARGET_R1, 0);
    if (right != -1) create_animation(b, x, first ? !(first = 0) : 0, MODULE_TARGET_R2, 0);

    if (left != -1) create_animation(b, left, first ? !(first = 0) : 0, MODULE_TARGET_R2, 0);
    if (right != -1) create_animation(b, right, first ? !(first = 0) : 0, MODULE_TARGET_R1, 0);

}

void deactivate_module(BarData* b, size_t x) {

    int left = -1;
    int right = -1;
    int module_width = b->modules[x].fields[MODULE_W];
    // Find left and right modules
    for (int i = (int)x - 1; i >= 0; i--) {
        if (b->modules[i].fields[MODULE_ACTIVE]) {
            left = i;
            break;
        }
    }
    for (size_t i = x + 1; i < b->modules_size; i++) {
        if (b->modules[i].fields[MODULE_ACTIVE]) {
            right = i;
            break;
        }
    }

    // round corners on adjacent modules
    if (left != -1) create_animation(b, left, 0, MODULE_TARGET_R2, 10);
    if (right != -1) create_animation(b, right, 0, MODULE_TARGET_R1, 10);
    if (left != -1) create_animation(b, x, 0, MODULE_TARGET_R1, 10);
    if (right != -1) create_animation(b, x, 0, MODULE_TARGET_R2, 10);

    create_animation(b, x, 1, MODULE_TARGET_Y, b->h);

    // move all previous modules
    int first = 1;
    for (size_t i = 0; i < x; i++) {
        create_animation(b, i, first, MODULE_TARGET_X, b->modules[i].fields[MODULE_X] + module_width/2);
        first = 0;
    }
    for (size_t i = x+1; i < b->modules_size; i++) {
        create_animation(b, i, 0, MODULE_TARGET_X, b->modules[i].fields[MODULE_X] - module_width/2);
    }

    create_animation(b, x, 1, MODULE_ACTIVE, 0);

    if (left != -1 && right != -1) {
        create_animation(b, left, 0, MODULE_TARGET_R2, 0);
        create_animation(b, right, 0, MODULE_TARGET_R1, 0);
    }
}

void action_queue(BarData* b, int action, int index) {
    AnimationNode* a = malloc(sizeof(AnimationNode));
    if (b->action_tail)
        b->action_tail->next = a;
    else
        b->action_head = a;
    b->action_tail = a;

    *(a) = (AnimationNode) {
        .module = index,
        .field = action,
        .value = index,
        .wait = 0,
        .next = NULL
    };
}

void create_module(BarData* b, int index, int width) {
    Module* m = b->modules + index;
    module_set_field(m, MODULE_ACTIVE, 0);
    module_set_field(m, MODULE_ID, index);
    module_set_dim(m, 0, 0, width, b->h);
    module_set_radi(m, 0, 0, 0, 0);

}

void bar_init(cairo_t* cairo, void* data) {
    BarData* b = data;
    b->layout = pango_cairo_create_layout(cairo);
    b->font = pango_font_description_from_string("Inter 13");
    pango_layout_set_font_description(b->layout, b->font);

    b->animation_head = NULL;
    b->animation_tail = NULL;
    b->action_head = NULL;
    b->action_tail = NULL;

    b->x = 0;
    b->y = 1440 - 60;
    b->w = 2560;
    b->h = 60;
    b->modules_size = 2;
    b->modules = calloc(b->modules_size, sizeof(Module));

    // create module
    int module_width = 200;
    create_module(b, 0, module_width);
    create_module(b, 1, module_width);
    clock_module(b->modules);
    playerctl_module(b->modules+1);

    for (size_t i = 0; i < b->modules_size; i++) {
        Module* m = b->modules + i;
        m->init(m);
    }

    // fill FDs
    b->fds_size = 0;
    for (size_t i = 0; i < b->modules_size; i++) {
        b->fds_size += b->modules[i].fds_size;
    }

    b->fds = calloc(b->fds_size, sizeof(int));
    b->fd_modules = calloc(b->fds_size, sizeof(int));

    int current = 0;
    for (size_t i = 0; i < b->modules_size; i++) {
        for(size_t j = 0; j < b->modules[i].fds_size; j++) {
            b->fds[current] = b->modules[i].fds[j];
            b->fd_modules[current++] = i;
        }
    }

    b->update = 1;
    bar_draw(cairo, data);
}

int lerp(float a, float b, float p)
{
    float ans = a * (1.0 - p) + (b * p);
    return (a - b < 0) ? ceil(ans) : floor(ans);
}

int goto_target(int* a, int current, int target) {
    if (a[current] != a[target]) {
        a[current] = lerp((float)a[current], (float)a[target], 0.2f);
        return 1;
    } 
    return 0;
}

int bar_step(int* active, void* data) {
    (void)active;
    BarData* b = data;
    int update = 0;

    // Animate to target position
    for (size_t i = 0; i < b->modules_size; i++) {
        Module* m = b->modules + i;
        int* a = m->fields;

        update |= goto_target(a, MODULE_X, MODULE_TARGET_X);
        update |= goto_target(a, MODULE_Y, MODULE_TARGET_Y);
        update |= goto_target(a, MODULE_W, MODULE_TARGET_W);
        update |= goto_target(a, MODULE_H, MODULE_TARGET_H);
        update |= goto_target(a, MODULE_R1, MODULE_TARGET_R1);
        update |= goto_target(a, MODULE_R2, MODULE_TARGET_R2);
        update |= goto_target(a, MODULE_R3, MODULE_TARGET_R3);
        update |= goto_target(a, MODULE_R4, MODULE_TARGET_R4);
    }

    if (update) {
        b->update = 1;
        return 0;
    }

    if (b->animation_head) {
        AnimationNode* a = b->animation_head;
        do {
            b->animation_head = b->animation_head->next;
            if (!b->animation_head) b->animation_tail = NULL;
            b->modules[a->module].fields[a->field] = a->value;
            free(a);
            a = b->animation_head;
        }
        while (a && !a->wait);
    } else if (b->action_head) {
        AnimationNode* a = b->action_head;
        b->action_head = b->action_head->next;
        if (!b->action_head) b->action_tail = NULL;

        if (a->field == ACTION_ACTIVATE) {
            activate_module(b, a->value);
        }
        if (a->field == ACTION_DEACTIVATE) {
            deactivate_module(b, a->value);
        }

        free(a);
    }

    return 0;
}


void cairo_rectangle_radius(cairo_t* cr, int x, int y, int w, int h, int r1, int r2, int r3, int r4) {
    if (r1 != 0) {
        cairo_move_to(cr, x, y + r1);
        cairo_arc(cr, x + r1, y + r1, r1, M_PI, 3 * M_PI / 2);
    } else {
        cairo_move_to(cr, x, y);
    }

    if (r2 != 0) {
        cairo_arc(cr, x + w - r2, y + r2, r2, -M_PI / 2, 0);
    } else {
        cairo_line_to(cr, x + w, y);
    }

    if (r3 != 0) {
        cairo_arc(cr, x + w - r3, y + h - r3, r3, 0, M_PI / 2);
    } else {
        cairo_line_to(cr, x + w, y + h);
    }

    if (r4 != 0) {
        cairo_arc(cr, x + r4, y + h - r4, r4, M_PI / 2, M_PI);
    } else {
        cairo_line_to(cr, x, y + h);
    }

    cairo_close_path(cr);
}

int bar_draw(cairo_t* cairo, void* data) {
    BarData* b = data;

    if (!b->update) return 1;

    cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 0.0);
    cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cairo);

    cairo_move_to(cairo, 0,0);
    for (size_t i = 0; i < b->modules_size; i++) {
        Module* m = b->modules + i;
        int* a = m->fields;
        if (!a[MODULE_ACTIVE]) continue;
        cairo_set_source_rgba(cairo, 0.2, 0.2, 0.2, 0.8);
        cairo_rectangle_radius(cairo, 
                               a[MODULE_X], 
                               a[MODULE_Y], 
                               a[MODULE_W], 
                               a[MODULE_H], 
                               a[MODULE_R1],
                               a[MODULE_R2],
                               a[MODULE_R3],
                               a[MODULE_R4]
                               );
        cairo_fill(cairo);
        m->draw(m, cairo, b->layout);
    }
    return 0;
}

void bar_destroy(void* data) {
    BarData* d = data;
    g_object_unref(d->layout);
    pango_font_description_free(d->font);
    free(d);
}

void bar_callback(int fd, void* data) {
    BarData* b = data;
    for (size_t i = 0; i < b->fds_size; i++) {
        if (b->fds[i] == fd) {
            Module* m = b->modules + b->fd_modules[i];
            int r = m->callback(m, fd, b->layout);
            if (r & MODULE_UPDATE) {
                b->update = 1;
            }
            if (r & MODULE_ACTIVATE) {
                action_queue(b, ACTION_ACTIVATE, m->fields[MODULE_ID]);
            }
            if (r & MODULE_DEACTIVATE) {
                action_queue(b, ACTION_DEACTIVATE, m->fields[MODULE_ID]);
            }
        }
    }
}

WidgetFD* bar_fds(void* data) {
    BarData* b = data;
    WidgetFD* fds = malloc(sizeof(WidgetFD));
    fds->size = b->fds_size;

    fds->fd = calloc(fds->size, sizeof(int));
    fds->callback = calloc(fds->size, sizeof &bar_callback);

    for (size_t i = 0; i < b->fds_size; i++) {
        fds->fd[i] = b->fds[i];
        fds->callback[i] = bar_callback;
    }

    return fds;
}

WidgetOps* bar() {
    WidgetOps* b = malloc(sizeof(WidgetOps));
    *b = (WidgetOps) {
        .init = bar_init,
        .draw = bar_draw,
        .keyboard = NULL,
        .step = bar_step,
        .destroy = bar_destroy,
        .on_toggle = NULL,
        .get_fds = bar_fds
    };
    return b;
}

