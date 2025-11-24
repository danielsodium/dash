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

    if (ftruncate(fd, size * 2) < 0) {
        perror("failed to size shared memory");
        close(fd);
        return NULL;
    }

    data = mmap(NULL, size * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("failed to map memory to process space");
        close(fd);
        return NULL;
    }
    c->data = data;
    c->data_size = size * 2;

    pool = wl_shm_create_pool(shm, fd, size * 2);
    
    c->buffers[0] = wl_shm_pool_create_buffer(pool, 0, width, height,
                                              stride, WL_SHM_FORMAT_ARGB8888);
    c->buffers[1] = wl_shm_pool_create_buffer(pool, size, width, height,
                                              stride, WL_SHM_FORMAT_ARGB8888);
    c->current_buffer = 0;
    
    wl_shm_pool_destroy(pool);
    close(fd);

    c->surfaces[0] = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32, 
                                                         width, height, stride);
    c->surfaces[1] = cairo_image_surface_create_for_data((unsigned char*)data + size, CAIRO_FORMAT_ARGB32,
                                                         width, height, stride);
    
    c->cairo = cairo_create(c->surfaces[0]);
    c->width = width;
    c->height = height;
    c->stride = stride;
    
    return c;
}

void canvas_swap_buffers(Canvas* c) {
    cairo_destroy(c->cairo);
    c->current_buffer = 1 - c->current_buffer;
    c->cairo = cairo_create(c->surfaces[c->current_buffer]);
}

struct wl_buffer* canvas_get_current_buffer(Canvas* c) {
    return c->buffers[c->current_buffer];
}

void canvas_destroy(Canvas* c) {
    cairo_surface_flush(c->surfaces[0]);
    cairo_surface_flush(c->surfaces[1]);
    cairo_destroy(c->cairo);
    cairo_surface_destroy(c->surfaces[0]);
    cairo_surface_destroy(c->surfaces[1]);
    munmap(c->data, c->data_size);
    wl_buffer_destroy(c->buffers[0]);
    wl_buffer_destroy(c->buffers[1]);
    free(c);
}
