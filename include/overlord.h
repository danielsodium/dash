#ifndef _OVERLORD_H_
#define _OVERLORD_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include <cairo/cairo.h>
#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include <xkbcommon/xkbcommon.h>

#include "canvas.h"
#include "loop.h"
#include "common.h"

struct Overlord {
    int active;

    Keyboard* keyboard;
    Widget* widget;
    Loop* loop;
};

int overlord_run(int sock);

#endif
