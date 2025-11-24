#include "drun.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>

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
    if (strlen(d->input) == 0) {
        d->options_size = 0;
        return;
    }

    d->options_size = 0;
    for (i = 0; i < d->bins_size; i++) {
        if (strstr(d->bins[i], d->input)) {
            strcpy(d->options[d->options_size++], d->bins[i]);
            if (d->options_size >= 5) break;
        }
    }
}

void drun_init(cairo_t* cairo, void* data) {
    DRunData* d = data;
    d->input[0] = '\0';
    d->layout = pango_cairo_create_layout(cairo);
    d->font = pango_font_description_from_string("Inter 20");
    pango_layout_set_font_description(d->layout, d->font);
    d->update = 1;

    drun_get_bin(d);
}

void rounded_rectangle(cairo_t *cr, double x, double y, double width, double height, double radius) {
    double degrees = M_PI / 180.0;

    cairo_new_sub_path(cr);
    cairo_arc(cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
    cairo_arc(cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
    cairo_arc(cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
    cairo_arc(cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path(cr);

    cairo_fill(cr);

}

void draw_gradient_rounded_rect(cairo_t* cr, double x, double y, double width, double height,
                                double corner_radius,
                                double r1, double g1, double b1,  // Start color
                                double r2, double g2, double b2) { // End color
    // Create a linear gradient from top-left to bottom-right
    cairo_pattern_t* gradient = cairo_pattern_create_linear(
        x, y,                    // Start point (top-left)
        x + width, y + height    // End point (bottom-right)
    );
    
    // Add color stops (0.0 = start, 1.0 = end)
    cairo_pattern_add_color_stop_rgb(gradient, 0.0, r1, g1, b1);
    cairo_pattern_add_color_stop_rgb(gradient, 1.0, r2, g2, b2);
    
    // Draw the rounded rectangle
    double degrees = M_PI / 180.0;
    
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + width - corner_radius, y + corner_radius, corner_radius, -90 * degrees, 0 * degrees);
    cairo_arc(cr, x + width - corner_radius, y + height - corner_radius, corner_radius, 0 * degrees, 90 * degrees);
    cairo_arc(cr, x + corner_radius, y + height - corner_radius, corner_radius, 90 * degrees, 180 * degrees);
    cairo_arc(cr, x + corner_radius, y + corner_radius, corner_radius, 180 * degrees, 270 * degrees);
    cairo_close_path(cr);
    
    cairo_set_source(cr, gradient);
    cairo_fill(cr);
    
    // Clean up
    cairo_pattern_destroy(gradient);
}

int drun_draw(cairo_t* cairo, void* data) {
    DRunData* d = data;

    // Clear background
    cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 0.0);
    cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cairo);
    cairo_set_operator(cairo, CAIRO_OPERATOR_OVER);

    cairo_move_to(cairo, 0, 0);
    cairo_set_source_rgba(cairo, 0.9,0.9,0.9,0.5);
    rounded_rectangle(cairo, 0, 0, 800,400, 10);
    cairo_move_to(cairo, 20, 0);
    cairo_set_source_rgb(cairo, 1, 1, 1);

    // SEARCH BAR
    cairo_set_source_rgba(cairo, 1.0,1.0,1.0,0.5);
    rounded_rectangle(cairo, 40, 40, 720, 60, 10);

    // RESULTS
    rounded_rectangle(cairo, 40, 120, 720, 240, 10);

    draw_gradient_rounded_rect(cairo, 710, 50, 40, 40, 5,
                          1.0, 0.0, 0.0,  // Red
                          1.0, 0.6, 0.4); // Blue
   

    cairo_set_source_rgba(cairo, 0.0,0.0,0.0,1.0);
    cairo_move_to(cairo, 80, 55);
    pango_layout_set_text(d->layout, d->input, -1);
    pango_cairo_update_layout(cairo, d->layout);
    pango_cairo_show_layout(cairo, d->layout);

    for (size_t i = 0; i < d->options_size; i++) {
        cairo_move_to(cairo, 80, 140+35*i);
        pango_layout_set_text(d->layout, d->options[i], -1);
        pango_cairo_update_layout(cairo, d->layout);
        pango_cairo_show_layout(cairo, d->layout);
    }

    return 0;
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
            drun_find(d);
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
    return 0;
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
