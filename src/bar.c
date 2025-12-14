#include "bar.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>

#include "status.h"

void create_section(Section* s, int x, int y, int w, int h) {
    *s = (Section) {
        .x = x, .y = y, .w = w, .h = h,
        .tx = x, .ty = y, .tw = w, .th = h
    };
}

void create_animation(BarData* b, Section* s, int wait, int dx, int dy, int dw, int dh) {
    QueueNode* a = malloc(sizeof(QueueNode));
    if (b->animation_tail)
        b->animation_tail->next = a;
    else
        b->animation_head = a;
    b->animation_tail = a;

    *(a) = (QueueNode) {
        .section = s,
        .dx = dx, .dy = dy, .dw = dw, .dh = dh,
        .wait = wait,
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

    b->x = 0;
    b->y = 1440 - 30;
    b->w = 2560;
    b->h = 50;
    b->sections_size = 2;
    b->sections = calloc(b->sections_size, sizeof(Section));
    create_section(b->sections, 0, 0, 100, 50);
    create_animation(b, b->sections, 1, 1000, 0, 0, 0);
    create_animation(b, b->sections, 1, -1000, 0, 0, 0);
    create_animation(b, b->sections, 1, 1000, 0, 0, 0);
    create_animation(b, b->sections, 1, -1000, 0, 0, 0);

    create_section(b->sections + 1, 550, 50, 100, 50);
    create_animation(b, b->sections + 1, 0, 0, -50, 0, 0);
}

int lerp(float a, float b, float p)
{
    float ans = a * (1.0 - p) + (b * p);
    return (a - b < 0) ? ceil(ans) : floor(ans);
}

int bar_step(int* active, void* data) {
    (void)active;
    BarData* b = data;
    b->update = 0;

    // Animate to target position
    for (size_t i = 0; i < b->sections_size; i++) {
        Section* s = b->sections + i;
        if (s->x != s->tx) {
            b->update = 1;
            s->x = lerp((float)s->x, (float)s->tx, 0.2f);
        } 
        if (s->y != s->ty) {
            b->update = 1;
            s->y = lerp((float)s->y, (float)s->ty, 0.2f);
        }
        if (s->w != s->tw) {
            b->update = 1;
            s->w = lerp((float)s->w, (float)s->tw, 0.2f);
        }
        if (s->h != s->th) {
            b->update = 1;
            s->h = lerp((float)s->h, (float)s->th, 0.2f);
        }
    }

    if (!b->update && b->animation_head) {
        QueueNode* a = b->animation_head;
        do {
            b->animation_head = b->animation_head->next;
            if (!b->animation_head) b->animation_tail = NULL;
            a->section->tx = a->section->x + a->dx;
            a->section->ty = a->section->y + a->dy;
            a->section->tw = a->section->w + a->dw;
            a->section->th = a->section->h + a->dh;
            free(a);
            a = b->animation_head;
        }
        while (a && !a->wait);
    }

    return 0;
}


void cairo_rectangle_radius(cairo_t* cr, int x, int y, int w, int h, int corners, int radius) {
    if (radius > w / 2) radius = w / 2;
    if (radius > h / 2) radius = h / 2;
    if (radius < 0) radius = 0;

    if (corners & (1 << CORNER_TOP_LEFT)) {
        cairo_move_to(cr, x, y + radius);
        cairo_arc(cr, x + radius, y + radius, radius, M_PI, 3 * M_PI / 2);
    } else {
        cairo_move_to(cr, x, y);
    }

    if (corners & (1 << CORNER_TOP_RIGHT)) {
        cairo_arc(cr, x + w - radius, y + radius, radius, -M_PI / 2, 0);
    } else {
        cairo_line_to(cr, x + w, y);
    }

    if (corners & (1 << CORNER_BOT_RIGHT)) {
        cairo_arc(cr, x + w - radius, y + h - radius, radius, 0, M_PI / 2);
    } else {
        cairo_line_to(cr, x + w, y + h);
    }

    if (corners & (1 << CORNER_BOT_LEFT)) {
        cairo_arc(cr, x + radius, y + h - radius, radius, M_PI / 2, M_PI);
    } else {
        cairo_line_to(cr, x, y + h);
    }

    cairo_close_path(cr);
}

int bar_draw(cairo_t* cairo, void* data) {
    BarData* b = data;

    if (!b->update) return 1;

    // background
    cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 0.0);
    cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cairo);

    cairo_move_to(cairo, 0,0);
    for (size_t i = 0; i < b->sections_size; i++) {
        Section* s = b->sections + i;
        cairo_set_source_rgb(cairo, 1, 1, 1);
        int rounded_corners = (1 << CORNER_TOP_LEFT) | (1 << CORNER_TOP_RIGHT);
        cairo_rectangle_radius(cairo, s->x, s->y, s->w, s->h, rounded_corners, 10);
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
