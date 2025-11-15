#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "window.h"
#include "draw.h"

int main() {
    Window* win;
    Canvas* c;

    win = window_init();
    c = draw_init(win);

    time_t last_update = 0;
    while (win->active) {
        time_t now = time(NULL);
        // every second
        if (now != last_update) {
            draw_bar(c);
            window_draw_buffer(win, c->buffer);
            last_update = now;
        }
        window_handle_events(win);
        usleep(100000);
    }

    window_destroy(win);

    return 0;
}
