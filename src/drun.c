#include "drun.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>

static void drun_str(char* str, DRunData* d) {
    size_t i;
    char buffer[33];
    char list[256];


    list[0] = '\0';
    for (i = 0; i < d->options_size; i++) {
        snprintf(buffer, WIDTH_CHARS + 1, (i == 0) ? "> %s\n" : "  %s\n", d->options[i]);
        strcat(list, buffer);
    }

    int input_diff = strlen(d->input) - WIDTH_CHARS + 3;
    char* last_chars = d->input + ((input_diff > 0) ? input_diff : 0);

    sprintf(str, "\n\n\ndrun\n$ %s\xE2\x96\x88\n%s", last_chars, list);
}

static void drun_get_bin(DRunData *d) {
    char* path, *p, *p_start, *dir_name;
    DIR* dir;

    d->bins_max = 32;
    d->bins = malloc(d->bins_max * sizeof(char*));
    d->bins_size = 0;
    d->options_size = 0;

    path = getenv("PATH");
    if (!path) {
        perror("Failed to get path");
        return;
    }
    p = strdup(path);
    p_start = p;

    dir_name = strtok(p, ":");
    while(dir_name) {
        dir = opendir(dir_name);
        if (!dir) {
            dir_name = strtok(NULL, ":");
            continue;
        }
        struct dirent *file;
        while ((file = readdir(dir))) {
            if (file->d_name[0] == '.')
                continue;
            char f_path[2048];
            snprintf(f_path, sizeof(f_path), "%s/%s", dir_name, file->d_name);

            if (access(f_path, X_OK) == 0) {
                struct stat st;
                if (stat(f_path, &st) == 0 && S_ISREG(st.st_mode)) {
                    if (d->bins_size == d->bins_max) {
                        d->bins_max *= 2;
                        d->bins = realloc(d->bins, d->bins_max * sizeof(char*));
                        if (!d->bins) {
                            perror("Failed realloc");
                            return;
                        }
                    }
                    d->bins[d->bins_size++] = strdup(file->d_name);
                }
            }
        }
        dir_name = strtok(NULL, ":");
    }
    free(p_start);
}

static void drun_find(DRunData* d) {
    size_t i;

    d->options_size = 0;
    for (i = 0; i < d->bins_size; i++) {
        if (strstr(d->bins[i], d->input)) {
            strcpy(d->options[d->options_size++], d->bins[i]);
            if (d->options_size >= 5) return;
        }
    }
}

void drun_init(cairo_t* cairo, void* data) {
    DRunData* d = data;
    d->input[0] = '\0';
    d->layout = pango_cairo_create_layout(cairo);
    d->font = pango_font_description_from_string("JetBrainsMono Nerd Font 18");
    pango_layout_set_font_description(d->layout, d->font);
    d->update = 1;

    drun_get_bin(d);
}

void drun_draw(cairo_t* cairo, int* active, void* data) {
    (void)active;
    DRunData* d = data;

    // Clear background
    cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 0.0);
    cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cairo);

    cairo_move_to(cairo, 0, 0);
    cairo_set_source_rgba(cairo, 1.0,1.0,1.0,1.0);
    char str[1024];
    drun_str(str, d);
    pango_layout_set_text(d->layout, str, -1);
    pango_cairo_update_layout(cairo, d->layout);
    pango_cairo_show_layout(cairo, d->layout);
}

int drun_on_key(KeyboardData* event_data, int* active, void* data) {
    if (event_data->event != KEYBOARD_EVENT_KEY_PRESS && 
        event_data->event != KEYBOARD_EVENT_KEY_REPEAT) 
        return 0;

    DRunData* d = data;
    xkb_keysym_t* key = event_data->key;

    if (*key == XKB_KEY_Escape) {
        *active = 0;
        return 0;
    }
    if (*key == XKB_KEY_BackSpace) {
        size_t len = strlen(d->input);
        if (len > 0) {
            d->input[len - 1] = '\0';
            d->update = 1;
        }
        return 1;
    }

    if (*key == XKB_KEY_Return) {
        char command[64];
        if (d->options_size >= 1) {
            sprintf(command, "%s &", d->options[0]);
            system(command);
            *active = 0;
            return 0;
        }
    }

    char c[8];
    int n = xkb_keysym_to_utf8(*key, c, sizeof c);
    if (n > 0 && isalnum((unsigned char) *c)) {
        strcat(d->input, c);
        drun_find(d);
        d->update = 1;
    }
    return 1;
}

void drun_destroy(void* data) {
    DRunData* d = data;
    size_t i;

    g_object_unref(d->layout);
    pango_font_description_free(d->font);
    
    for (i = 0; i < d->bins_size; i++) {
        free(d->bins[i]);
    }
    free(d->bins);
    free(d);
}
