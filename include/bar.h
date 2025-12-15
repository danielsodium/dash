#ifndef _BAR_H_
#define _BAR_H_

#include <cairo/cairo.h>
#include <xkbcommon/xkbcommon.h>
#include <pango/pangocairo.h>

#include "widget.h"

typedef enum SectionArgs {
    SECTION_X = 0,
    SECTION_Y,
    SECTION_W,
    SECTION_H,
    SECTION_TARGET_X,
    SECTION_TARGET_Y,
    SECTION_TARGET_W,
    SECTION_TARGET_H,
    SECTION_R1,
    SECTION_R2,
    SECTION_R3,
    SECTION_R4,
    SECTION_TARGET_R1,
    SECTION_TARGET_R2,
    SECTION_TARGET_R3,
    SECTION_TARGET_R4
} SectionArgs;

typedef struct {
    int active;
    int args[16];
} Section;

typedef struct QueueNode {
    Section* section;
    int property;
    int value;
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

int bar_draw(cairo_t* cairo, void* data);

WidgetOps* bar();

#endif
