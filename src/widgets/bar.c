#include "bar.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>

#include "status.h"
#include "util.h"


void create_animation(BarData* b, int module_num, int flags, int* field, int value) {
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
        .flags = flags,
        .next = NULL
    };
}

void activate_module(BarData* b, size_t x) {
    int left = -1;
    int right = -1;
    Module* m = b->modules;
    // Find left and right modules
    for (int i = (int)x - 1; i >= 0; i--) {
        if (m[i].active && m[i].align == m[x].align) {
            left = i;
            break;
        }
    }
    for (size_t i = x + 1; i < b->modules_size; i++) {
        if (m[i].active && m[i].align == m[x].align) {
            right = i;
            break;
        }
    }

    // round the inserting module
    module_set_radi(m + x, 10, 10, 0, 0);

    // round corner on adjacent modules
    if (left != -1) create_animation(b, left, 0, &m[left].r2, 10);
    if (right != -1) create_animation(b, right, 0, &m[right].r1, 10);

    if (m[x].align == MODULE_ALIGN_LEFT) {
        // move all previous modules
        for (size_t i = x+1; i < b->modules_size; i++) {
            if (m[i].align == m[x].align)
                create_animation(b, i, 0, &m[i].x, m[i].x + m[x].w);
        }
        create_animation(b, 0, 0, &b->spacers[0].x, b->spacers[0].x + m[x].w);
        create_animation(b, 0, 0, &b->spacers[0].w, b->spacers[0].w - m[x].w);

        // Get x position
        if (left == -1) 
            create_animation(b, x, 0, &m[x].x, (b->x - b->w)/2);
        else if (right == -1)
            create_animation(b, x, 0, &m[x].x, b->spacers[0].x);
        else
            create_animation(b, x, 0, &m[x].x, m[right].x);

    } else if (m[x].align == MODULE_ALIGN_CENTER) {
    } else if (m[x].align == MODULE_ALIGN_RIGHT) {
        for (size_t i = 0; i < x; i++) {
            if (m[i].align == m[x].align)
                create_animation(b, i, 1, &m[i].x, m[i].x - m[x].w);
        }
        printf("%d\n", m[x].w);
        create_animation(b, 0, 0, &b->spacers[1].w, b->spacers[1].w - m[x].w);

        // Get x position
        if (right == -1) 
            create_animation(b, x, 0, &m[x].x, (b->x + b->w)/2 - m[x].w);
        else if (left == -1)
            create_animation(b, x, 0, &m[x].x, b->spacers[1].x + b->spacers[1].w - m[x].w);
        else
            create_animation(b, x, 0, &m[x].x, m[left].x);
    }

    // MAKE THIS TOP ONE INSTANT
    create_animation(b, x, 1, &m[x].y, b->h);
    create_animation(b, x, 1, &m[x].active, 1);
    create_animation(b, x, 1, &m[x].y, 0);

    int first = 1;
    if (left != -1 || m[x].align == MODULE_ALIGN_RIGHT) 
        create_animation(b, x, first ? !(first = 0) : 0, &m[x].r1, 0);
    if (right != -1 || m[x].align == MODULE_ALIGN_LEFT) 
        create_animation(b, x, first ? !(first = 0) : 0, &m[x].r2, 0);

    if (left != -1) create_animation(b, left, first ? !(first = 0) : 0, &m[left].r2, 0);
    else if (m[x].align == MODULE_ALIGN_RIGHT) 
        create_animation(b, x, first ? !(first = 0) : 0, &b->spacers[1].r2, 0);

    if (right != -1) create_animation(b, right, first ? !(first = 0) : 0, &m[right].r1, 0);
    else if (m[x].align == MODULE_ALIGN_LEFT) 
        create_animation(b, x, first ? !(first = 0) : 0, &b->spacers[0].r1, 0);

    create_animation(b, x, 1, &m[x].activate_queued, 1);

}

void deactivate_module(BarData* b, size_t x) {

    int left = -1;
    int right = -1;
    Module* m = b->modules;
    // Find left and right modules
    for (int i = (int)x - 1; i >= 0; i--) {
        if (m[i].active && m[i].align == m[x].align) {
            left = i;
            break;
        }
    }
    for (size_t i = x + 1; i < b->modules_size; i++) {
        if (m[i].active && m[i].align == m[x].align) {
            right = i;
            break;
        }
    }

    // round corners on adjacent modules
    if (left != -1)
        create_animation(b, left, 0, &m[left].r2, 10);
    else if (m[x].align == MODULE_ALIGN_RIGHT)
        create_animation(b, left, 0, &b->spacers[1].r2, 10);

    if (right != -1)
        create_animation(b, right, 0, &m[right].r1, 10);
    else if (m[x].align == MODULE_ALIGN_LEFT)
        create_animation(b, right, 0, &b->spacers[0].r1, 10);

    int first = 1;
    if (left != -1) create_animation(b, x, 0, &m[x].r1, 10);
    else if (m[x].align == MODULE_ALIGN_RIGHT) 
        create_animation(b, x, first ? !(first = 0) : 0, &b->spacers[1].r2, 10);

    if (right != -1) create_animation(b, x, 0, &m[x].r2, 10);
    else if (m[x].align == MODULE_ALIGN_LEFT) 
        create_animation(b, x, first ? !(first = 0) : 0, &b->spacers[1].r1, 10);

    create_animation(b, x, 1, &m[x].y, b->h);

    first = 1;
    if (m[x].align == MODULE_ALIGN_LEFT) {
        // move all previous modules
        for (size_t i = x+1; i < b->modules_size; i++) {
            if (m[i].align == m[x].align)
                create_animation(b, i, first ? !(first = 0) : 0, &m[i].x, m[i].x - m[x].w);
        }
        create_animation(b, 0, first ? !(first = 0) : 0, &b->spacers[0].x, b->spacers[0].x - m[x].w);
        create_animation(b, 0, 0, &b->spacers[0].w, b->spacers[0].w + m[x].w);
    } else if (m[x].align == MODULE_ALIGN_CENTER) {
    } else if (m[x].align == MODULE_ALIGN_RIGHT) {
        for (size_t i = 0; i < x; i++) {
            if (m[i].align == m[x].align)
                create_animation(b, i, first ? !(first = 0) : 0, &m[i].x, m[i].x + m[x].w);
        }
        create_animation(b, 0, first ? !(first = 0) : 0, &b->spacers[1].w, b->spacers[0].w + m[x].w);
    }

    create_animation(b, x, 1, &m[x].active, 0);

    if (left != -1 && right != -1) {
        create_animation(b, left, 0, &m[left].r2, 0);
        create_animation(b, right, 0, &m[right].r1, 0);
    } else if (left != -1 && m[x].align == MODULE_ALIGN_RIGHT) {
        create_animation(b, right, 0, &m[right].r1, 0);
        create_animation(b, right, 0, &b->spacers[1].r2, 0);
    } else if (right != -1 && m[x].align == MODULE_ALIGN_LEFT) {
        create_animation(b, right, 0, &m[left].r2, 0);
        create_animation(b, right, 0, &b->spacers[0].r1, 0);
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
        .field = NULL,
        .value = index,
        .flags = action,
        .next = NULL
    };
}

void create_module(BarData* b, int index, int width) {
    Module* m = b->modules + index;
    m->active = 0;
    m->activate_queued = 0;
    m->id = index;
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

    b->x = 2560;
    b->y = 1440 - 60;
    b->w = 1000;
    b->h = 60;
    b->modules_size = 2;
    b->modules = calloc(b->modules_size, sizeof(Module));

    b->spacers[0] = (BarSpacer) {
        .x = (b->x - b->w)/2,
        .y = 0,
        .w = b->w/2,
        .h = b->y,
        .r1 = 10,
    };
    b->spacers[1] = (BarSpacer) {
        .x = b->x/2,
        .y = 0,
        .w = b->w/2,
        .h = b->y,
        .r2 = 10
    };

    // create module
    int module_width = 200;
    create_module(b, 0, module_width);
    create_module(b, 1, module_width);
    //create_module(b, 2, module_width);
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

    if (b->animation_head) {
        AnimationNode* c = b->animation_head;
        AnimationNode* last = NULL;
        do {
            if (*(c->field) == c->value) {
                AnimationNode* remove = c;
                if (remove == b->animation_head)
                    b->animation_head = b->animation_head->next;
                if (remove == b->animation_tail)
                    b->animation_tail = last;
                if (last)
                    last->next = remove->next;
                c = c->next;
                free(remove);
            } else {
                *(c->field) = lerp((float)*(c->field), (float)c->value, 0.2f);
                update = 1;
                last = c;
                c = c->next;
            }
        }
        while (c && !c->flags);
    }

    if (update) {
        b->update = 1;
        return 0;
    }

    if (b->action_head) {
        AnimationNode* a = b->action_head;
        b->action_head = b->action_head->next;
        if (!b->action_head) b->action_tail = NULL;
        Module* m = b->modules + a->value;

        if (a->flags == ACTION_ACTIVATE && !m->active && !m->activate_queued) {
            m->activate_queued = 1;
            activate_module(b, a->value);
        }
        if (a->flags == ACTION_DEACTIVATE && m->active) {
            deactivate_module(b, a->value);
        }

        free(a);
    }

    return 0;
}

int bar_draw(cairo_t* cairo, void* data) {
    BarData* b = data;

    if (!b->update) return 1;

    cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 0.0);
    cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cairo);

    for (size_t i = 0; i < 2; i++) {
        BarSpacer* m = b->spacers + i;
        cairo_move_to(cairo, 0,0);
        cairo_set_source_rgba(cairo, 0.2, 0.2, 0.2, 0.8);
        cairo_rectangle_radius(cairo, 
                               m->x, m->y, m->w, m->h,
                               m->r1, m->r2, m->r3, m->r4);
        cairo_fill(cairo);
    }

    for (size_t i = 0; i < b->modules_size; i++) {
        Module* m = b->modules + i;
        if (!m->active) continue;
        cairo_move_to(cairo, 0,0);
        cairo_set_source_rgba(cairo, 0.2, 0.2, 0.2, 0.8);
        cairo_rectangle_radius(cairo, 
                               m->x, m->y, m->w, m->h,
                               m->r1, m->r2, m->r3, m->r4);
        cairo_fill(cairo);
        if (m->draw) m->draw(m, cairo, b->layout);
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
                action_queue(b, ACTION_ACTIVATE, m->id);
            }
            if (r & MODULE_DEACTIVATE) {
                action_queue(b, ACTION_DEACTIVATE, m->id);
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

