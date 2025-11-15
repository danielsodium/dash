#include "widgets.h"

#include <time.h>
#include <string.h>

#include "status.h"

void system_widget(char* s) {
    s = print_cpu(s);
    *(s++) = '\n';
    s = print_mem(s);
    *(s++) = '\n';
    s = print_battery(s);
    *(s-1) = '\0';
}

void clock_widget(char* s) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "   %I:%M %p\n   %A", tm_info);
    strcpy(s, buffer);
}

void drun(char* s) {
    strcpy(s, "drun:\n------------\n\xE2\x96\x88");
}

