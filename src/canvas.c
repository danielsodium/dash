#include "canvas.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

Canvas* canvas_create(int width, int height, struct wl_shm* shm) {
    Canvas* c;
    struct wl_shm_pool* pool;
    char shm_name[32];
    int stride, size, fd;
    void* data;

    c = malloc(sizeof(Canvas));

    stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
    size = stride * height;

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
    munmap(data, size);

    pool = wl_shm_create_pool(shm, fd, size);
    c->buffer = wl_shm_pool_create_buffer(pool, 0, width, height,
                                          stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);

    c->surface = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32, 
                                                     width, height, stride);

    return c;
}

void canvas_start_buffer(Canvas* c) {
    c->cairo = cairo_create(c->surface);
}

void canvas_draw_buffer(Canvas* c) {
    cairo_destroy(c->cairo);
}

void canvas_destroy(Canvas* c) {
    wl_buffer_destroy(c->buffer);
    cairo_surface_destroy(c->surface);
    free(c);
}
