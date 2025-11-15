#include "draw.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <pango/pangocairo.h>

#include "status.h"

#define WIDTH 180
#define HEIGHT 1440

Canvas* draw_init(Window* w) {
    Canvas* c;
    struct wl_shm_pool* pool;
    char shm_name[32];
    int stride, size, fd;
    void* data;

    c = malloc(sizeof(Canvas));

    stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, WIDTH);
    size = stride * HEIGHT;

    snprintf(shm_name, sizeof(shm_name), "/dash-%d", getpid());

    fd = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd < 0) {
        perror("failed to create shared memory");
        return NULL;
    }
    shm_unlink(shm_name);

    if (ftruncate(fd, size) < 0) {
        perror("failed to size shared memory");
        close(fd);
        return NULL;
    }

    data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("failed to map memory to process space");
        close(fd);
        return NULL;
    }

    pool = wl_shm_create_pool(w->shm, fd, size);
    c->buffer = wl_shm_pool_create_buffer(pool, 0, WIDTH, HEIGHT,
                                                          stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);

    c->surface = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32, WIDTH,HEIGHT, stride);

    return c;
}

static void get_widgets_top(char* o) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "   %I:%M %p\n   %A", tm_info);
    strcpy(o, buffer);
}

static void get_widgets_bottom(char* o) {
    o = print_cpu(o);
    *(o++) = '\n';
    o = print_mem(o);
    *(o++) = '\n';
    o = print_battery(o);
    *(o-1) = '\0';
}

void draw_bar(Canvas* c) {
    cairo_t *cairo = cairo_create(c->surface);
    int text_width, text_height;
    char widgets[1024];

    // Clear background
    cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 0.0);
    cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cairo);

    PangoLayout *layout = pango_cairo_create_layout(cairo);
    PangoFontDescription *desc = pango_font_description_from_string("JetBrainsMono Nerd Font 18");
    pango_layout_set_font_description(layout, desc);
    get_widgets_top(widgets);
    pango_layout_set_text(layout, widgets, -1);
    pango_layout_get_pixel_size(layout, &text_width, &text_height);

    cairo_set_source_rgba(cairo, 1.0,1.0,1.0,1.0);
    cairo_move_to(cairo, 0, 0);
    pango_cairo_show_layout(cairo, layout);

    get_widgets_bottom(widgets);
    pango_layout_set_text(layout, widgets, -1);
    pango_layout_get_pixel_size(layout, &text_width, &text_height);
    cairo_move_to(cairo, 10, (HEIGHT - text_height - 10));
    pango_cairo_show_layout(cairo, layout);

    pango_font_description_free(desc);
    g_object_unref(layout);
    cairo_destroy(cairo);
}


