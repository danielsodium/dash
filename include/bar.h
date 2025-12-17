#ifndef _BAR_H_
#define _BAR_H_

#include <cairo/cairo.h>
#include <xkbcommon/xkbcommon.h>
#include <pango/pangocairo.h>

#include "widget.h"
#include "module.h"

typedef enum Actions {
    ACTION_ACTIVATE,
    ACTION_DEACTIVATE
} Actions;

typedef enum AnimationFlags {
    ANIMATION_WAIT = 1,
    ANIMATION_INSTANT = 2
} AnimationFlags;

typedef struct AnimationNode {
    int module;
    int* field;
    int value;
    int flags;
    struct AnimationNode* next;
} AnimationNode;

typedef struct {
    int update;
    int x, y, w, h;

    Module* modules;
    size_t modules_size;

    int* fds;
    int* fd_modules;
    size_t fds_size;

    AnimationNode* animation_head;
    AnimationNode* animation_tail;

    AnimationNode* action_head;
    AnimationNode* action_tail;

    PangoLayout* layout;
    PangoFontDescription* font;

} BarData;

int bar_draw(cairo_t* cairo, void* data);

WidgetOps* bar();

#endif
