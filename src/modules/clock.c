#include "module.h"

#include <time.h>
#include <sys/timerfd.h>
#include <unistd.h>

static int open_fd() {
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (fd == -1) {
        perror("Failed to create timer");
        return -1;
    }

    struct itimerspec spec = {0};
    spec.it_value.tv_sec = 1;
    spec.it_value.tv_nsec = 100;
    spec.it_interval = spec.it_value;
    
    if (timerfd_settime(fd, 0, &spec, NULL) == -1) {
        close(fd);
        return -1;
    }
    return fd;
}

static void init(Module* m) {
    ClockData* d = malloc(sizeof(ClockData));
    m->data = (void*) d;

    m->fields[MODULE_W] = 90;
    m->fields[MODULE_TARGET_W] = 90;

    m->fds_size = 1;
    m->fds = malloc(sizeof(int));
    m->fds[0] = open_fd();
}

static void draw(Module* m, cairo_t* cairo, PangoLayout* layout) {
    ClockData* d = m->data;
    cairo_move_to(cairo, m->fields[MODULE_X] + 20, m->fields[MODULE_Y] + 15);
    cairo_set_source_rgba(cairo, 1.0,1.0,1.0,1.0);
    pango_layout_set_text(layout, d->time, -1);
    pango_cairo_update_layout(cairo, layout);
    pango_cairo_show_layout(cairo, layout);
}

static int callback(Module* m, int fd) {
    uint64_t exp;
    read(fd, &exp, sizeof(exp));
    ClockData* c = m->data;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    sprintf(c->time, "%02d:%02d", t->tm_hour, t->tm_min);

    if (!m->fields[MODULE_ACTIVE]) return MODULE_ACTIVATE | MODULE_UPDATE;
    return MODULE_UPDATE;
}

void clock_module(Module* m) {
    m->init = init;
    m->draw = draw;
    m->callback = callback;
}
