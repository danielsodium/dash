#include "draw.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include "config.h"

Canvas* canvas_init(struct wl_shm* shm) {
    Canvas* c;
    struct wl_shm_pool* pool;
    char shm_name[32];
    int stride, size, fd;
    void* data;

    c = malloc(sizeof(Canvas));

    stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, WIDTH_PIXELS);
    size = stride * HEIGHT_PIXELS;

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

    pool = wl_shm_create_pool(shm, fd, size);
    c->buffer = wl_shm_pool_create_buffer(pool, 0, WIDTH_PIXELS, HEIGHT_PIXELS,
                                                          stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);

    c->surface = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32, WIDTH_PIXELS, HEIGHT_PIXELS, stride);

    c->font = pango_font_description_from_string("JetBrainsMono Nerd Font 13");

    return c;
}

void draw_start(Canvas* c) {
    c->cairo = cairo_create(c->surface);

    // Clear background
    cairo_set_source_rgba(c->cairo, 0.0, 0.0, 0.0, 0.0);
    cairo_set_operator(c->cairo, CAIRO_OPERATOR_SOURCE);
    cairo_paint(c->cairo);

    c->layout = pango_cairo_create_layout(c->cairo);
    pango_layout_set_font_description(c->layout, c->font);
}

void draw_to_buffer(Canvas* c) {
    g_object_unref(c->layout);
    cairo_destroy(c->cairo);
}

void canvas_destroy(Canvas* c) {
    pango_font_description_free(c->font);
}

void draw_string(Canvas* c, char* str, Anchor a) {
    int text_width, text_height;

    pango_layout_set_text(c->layout, str, -1);
    pango_layout_get_pixel_size(c->layout, &text_width, &text_height);
    cairo_set_source_rgba(c->cairo, 1.0,1.0,1.0,1.0);

    switch (a) {
        case ANCHOR_TOP:
            cairo_move_to(c->cairo, 0, 0);
            break;
        case ANCHOR_BOTTOM:
            cairo_move_to(c->cairo, 10, HEIGHT_PIXELS - text_height - 10);
            break;
        case ANCHOR_CENTER:
            cairo_move_to(c->cairo, 10, (HEIGHT_PIXELS - text_height/2 - 10)/4);
            break;
        default:
            cairo_move_to(c->cairo, 0, 0);
            break;
    }

    pango_cairo_show_layout(c->cairo, c->layout);
}
