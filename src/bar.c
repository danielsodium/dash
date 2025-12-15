#include "bar.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>

#include "status.h"

void section_set(Section* s, int property, int value) {
    s->args[property] = value;
}

void section_set_dim(Section* s, int x, int y, int w, int h) {
    section_set(s, SECTION_X, x);
    section_set(s, SECTION_Y, y);
    section_set(s, SECTION_W, w);
    section_set(s, SECTION_H, h);

    section_set(s, SECTION_TARGET_X, x);
    section_set(s, SECTION_TARGET_Y, y);
    section_set(s, SECTION_TARGET_W, w);
    section_set(s, SECTION_TARGET_H, h);
}

void section_set_radi(Section* s, int r1, int r2, int r3, int r4) {
    section_set(s, SECTION_R1, r1);
    section_set(s, SECTION_R2, r2);
    section_set(s, SECTION_R3, r3);
    section_set(s, SECTION_R4, r4);

    section_set(s, SECTION_TARGET_R1, r1);
    section_set(s, SECTION_TARGET_R2, r2);
    section_set(s, SECTION_TARGET_R3, r3);
    section_set(s, SECTION_TARGET_R4, r4);
}

void create_animation(BarData* b, int section_num, int wait, int property, int value) {
    AnimationNode* a = malloc(sizeof(AnimationNode));
    if (b->animation_tail)
        b->animation_tail->next = a;
    else
        b->animation_head = a;
    b->animation_tail = a;

    *(a) = (AnimationNode) {
        .section = b->sections + section_num,
        .property = property,
        .value = value,
        .wait = wait,
        .next = NULL
    };
}

void activate_section(BarData* b, size_t x) {

    int left = -1;
    int right = -1;
    int section_width = b->sections[x].args[SECTION_W];
    // Find left and right sections
    for (int i = (int)x - 1; i >= 0; i--) {
        if (b->sections[i].args[SECTION_ACTIVE]) {
            left = i;
            break;
        }
    }
    for (size_t i = x + 1; i < b->sections_size; i++) {
        if (b->sections[i].args[SECTION_ACTIVE]) {
            right = i;
            break;
        }
    }

    // round the inserting section
    section_set_radi(b->sections + x, 10, 10, 0, 0);

    // round corner on adjacent sections
    if (left != -1) create_animation(b, left, 0, SECTION_TARGET_R2, 10);
    if (right != -1) create_animation(b, right, 0, SECTION_TARGET_R1, 10);

    // move all previous sections left
    int first = 1;
    for (size_t i = 0; i < x; i++) {
        create_animation(b, i, first, SECTION_TARGET_X, b->sections[i].args[SECTION_X] - section_width/2);
        first = 0;
    }
    for (size_t i = x+1; i < b->sections_size; i++) {
        create_animation(b, i, 0, SECTION_TARGET_X, b->sections[i].args[SECTION_X] + section_width/2);
    }

    if (left == -1 && right == -1) {
        create_animation(b, x, 0, SECTION_X, b->w/2 - section_width/2);
        create_animation(b, x, 0, SECTION_TARGET_X, b->w/2 - section_width/2);
    } else if (right == -1) {
        create_animation(b, x, 0, SECTION_X, b->sections[left].args[SECTION_X] + b->sections[left].args[SECTION_W] - section_width/2);
        create_animation(b, x, 0, SECTION_TARGET_X, b->sections[left].args[SECTION_X] + b->sections[left].args[SECTION_W] - section_width/2);
    } else {
        create_animation(b, x, 0, SECTION_X, b->sections[right].args[SECTION_X] - section_width/2);
        create_animation(b, x, 0, SECTION_TARGET_X, b->sections[right].args[SECTION_X] - section_width/2);
    }

    create_animation(b, x, 1, SECTION_TARGET_Y, 0);
    create_animation(b, x, 1, SECTION_ACTIVE, 1);

    if (left != -1) create_animation(b, x, 0, SECTION_TARGET_R1, 0);
    if (right != -1) create_animation(b, x, 0, SECTION_TARGET_R2, 0);

    if (left != -1) create_animation(b, left, 0, SECTION_TARGET_R2, 0);
    if (right != -1) create_animation(b, right, 0, SECTION_TARGET_R1, 0);

}

void deactivate_section(BarData* b, size_t x) {

    int left = -1;
    int right = -1;
    int section_width = b->sections[x].args[SECTION_W];
    // Find left and right sections
    for (int i = (int)x - 1; i >= 0; i--) {
        if (b->sections[i].args[SECTION_ACTIVE]) {
            left = i;
            break;
        }
    }
    for (size_t i = x + 1; i < b->sections_size; i++) {
        if (b->sections[i].args[SECTION_ACTIVE]) {
            right = i;
            break;
        }
    }

    // round corners on adjacent sections
    if (left != -1) create_animation(b, left, 0, SECTION_TARGET_R2, 10);
    if (right != -1) create_animation(b, right, 0, SECTION_TARGET_R1, 10);
    if (left != -1) create_animation(b, x, 0, SECTION_TARGET_R1, 0);
    if (right != -1) create_animation(b, x, 0, SECTION_TARGET_R2, 0);

    create_animation(b, x, 1, SECTION_TARGET_Y, b->h);

    // move all previous sections
    int first = 1;
    for (size_t i = 0; i < x; i++) {
        create_animation(b, i, first, SECTION_TARGET_X, b->sections[i].args[SECTION_X] + section_width/2);
        first = 0;
    }
    for (size_t i = x+1; i < b->sections_size; i++) {
        create_animation(b, i, 0, SECTION_TARGET_X, b->sections[i].args[SECTION_X] - section_width/2);
    }

    create_animation(b, x, 1, SECTION_ACTIVE, 0);

    if (left != -1) create_animation(b, left, 0, SECTION_TARGET_R2, 0);
    if (right != -1) create_animation(b, right, 0, SECTION_TARGET_R1, 0);

}


void action_queue(BarData* b, int action, int index) {
    AnimationNode* a = malloc(sizeof(AnimationNode));
    if (b->action_tail)
        b->action_tail->next = a;
    else
        b->action_head = a;
    b->action_tail = a;

    *(a) = (AnimationNode) {
        .section = b->sections + index,
        .property = action,
        .value = index,
        .wait = 0,
        .next = NULL
    };
}

void bar_init(cairo_t* cairo, void* data) {
    BarData* b = data;
    b->layout = pango_cairo_create_layout(cairo);
    b->font = pango_font_description_from_string("JetBrainsMono Nerd Font 18");
    pango_layout_set_font_description(b->layout, b->font);

    b->animation_head = NULL;
    b->animation_tail = NULL;
    b->action_head = NULL;
    b->action_tail = NULL;

    b->x = 0;
    b->y = 1440 - 30;
    b->w = 2560;
    b->h = 50;
    b->sections_size = 4;
    b->sections = calloc(b->sections_size, sizeof(Section));

    // create section
    int section_width = 200;
    section_set_dim(b->sections, 0, 50, section_width, 50);
    section_set_dim(b->sections + 1, 0, 50, section_width, 50);
    section_set_dim(b->sections + 2, 0, 50, section_width, 50);
    section_set_dim(b->sections + 3, 0, 50, section_width, 50);

    action_queue(b, ACTION_ACTIVATE, 0);
    action_queue(b, ACTION_ACTIVATE, 2);
    action_queue(b, ACTION_ACTIVATE, 1);
    action_queue(b, ACTION_ACTIVATE, 3);
    action_queue(b, ACTION_DEACTIVATE, 1);

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
    b->update = 0;

    // Animate to target position
    for (size_t i = 0; i < b->sections_size; i++) {
        Section* s = b->sections + i;
        int* a = s->args;

        b->update |= goto_target(a, SECTION_X, SECTION_TARGET_X);
        b->update |= goto_target(a, SECTION_Y, SECTION_TARGET_Y);
        b->update |= goto_target(a, SECTION_W, SECTION_TARGET_W);
        b->update |= goto_target(a, SECTION_H, SECTION_TARGET_H);
        b->update |= goto_target(a, SECTION_R1, SECTION_TARGET_R1);
        b->update |= goto_target(a, SECTION_R2, SECTION_TARGET_R2);
        b->update |= goto_target(a, SECTION_R3, SECTION_TARGET_R3);
        b->update |= goto_target(a, SECTION_R4, SECTION_TARGET_R4);
    }

    if (b->update) return 0;

    if (b->animation_head) {
        AnimationNode* a = b->animation_head;
        do {
            b->animation_head = b->animation_head->next;
            if (!b->animation_head) b->animation_tail = NULL;
            a->section->args[a->property] = a->value;
            free(a);
            a = b->animation_head;
        }
        while (a && !a->wait);
    } else if (b->action_head) {
        AnimationNode* a = b->action_head;
        b->action_head = b->action_head->next;
        if (!b->action_head) b->action_tail = NULL;

        if (a->property == ACTION_ACTIVATE) {
            activate_section(b, a->value);
        }
        if (a->property == ACTION_DEACTIVATE) {
            deactivate_section(b, a->value);
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

    cairo_set_source_rgba(cairo, 1, 1, 1, 0.7);
    cairo_move_to(cairo, 0,0);
    for (size_t i = 0; i < b->sections_size; i++) {
        Section* s = b->sections + i;
        int* a = s->args;
        cairo_rectangle_radius(cairo, 
                               a[SECTION_X], 
                               a[SECTION_Y], 
                               a[SECTION_W], 
                               a[SECTION_H], 
                               a[SECTION_R1],
                               a[SECTION_R2],
                               a[SECTION_R3],
                               a[SECTION_R4]
                               );
        cairo_fill(cairo);
    }

    return 0;
}

void bar_destroy(void* data) {
    BarData* d = data;
    g_object_unref(d->layout);
    pango_font_description_free(d->font);
    free(d);
}

WidgetOps* bar() {
    WidgetOps* b = malloc(sizeof(WidgetOps));
    *b = (WidgetOps) {
        .init = bar_init,
        .draw = bar_draw,
        .keyboard = NULL,
        .step = bar_step,
        .destroy = bar_destroy,
        .on_toggle = NULL
    };
    return b;
}
