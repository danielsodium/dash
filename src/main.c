#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "window.h"
#include "draw.h"

int main() {

    Window* win;
    time_t last_update, now;

    win = window_init();
    last_update = 0;
    while (win->active) {
        now = time(NULL);
        // every second
        if (now != last_update) {
            window_draw(win);
            last_update = now;
        }
        window_handle_events(win);
        usleep(100000);
    }
    window_destroy(win);

    return 0;
}
