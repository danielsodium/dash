#ifndef _BAR_H_
#define _BAR_H_

#include <cairo/cairo.h>
#include <xkbcommon/xkbcommon.h>
#include <pango/pangocairo.h>

#include "widget.h"

typedef enum RadiusCorners {
    CORNER_TOP_LEFT = 0,
    CORNER_TOP_RIGHT,
    CORNER_BOT_LEFT,
    CORNER_BOT_RIGHT
} RadiusCorners;

typedef struct {
    int x, y, w, h, tx, ty, tw, th;
} Section;

typedef struct QueueNode {
    Section* section;
    int dx, dy, dw, dh, dr;
    int wait;
    struct QueueNode* next;
} QueueNode;

typedef struct {
    int update;
    int x, y, w, h;

    Section* sections;
    size_t sections_size;

    QueueNode* animation_head;
    QueueNode* animation_tail;

    PangoLayout* layout;
    PangoFontDescription* font;

} BarData;


WidgetOps* bar();

#endif
