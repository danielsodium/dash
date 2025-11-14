#include <stdio.h>

#include "window.h"

int main() {
    Window* win;

    win = window_init(150, 1440);
    window_destroy(win);

    return 0;
}
