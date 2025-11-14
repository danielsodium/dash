#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include "wlr-layer-shell-unstable-v1-client-protocol.h"

#define HEIGHT 1440
#define WIDTH 180 
#define BAR_BG_COLOR 0.0, 0.0, 0.0, 0.0
#define BAR_FG_COLOR 1.0, 1.0, 1.0, 1.0

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_shm *shm = NULL;
struct wl_surface *surface = NULL;
struct zwlr_layer_shell_v1 *layer_shell = NULL;
struct zwlr_layer_surface_v1 *layer_surface = NULL;
struct wl_output *output = NULL;

int running = 1;

static void noop(void *data, struct wl_registry *registry, uint32_t name) {
    (void)data;
    (void)registry;
    (void)name;
}

static void layer_surface_configure(void *data,
                                   struct zwlr_layer_surface_v1 *surface,
                                   uint32_t serial, uint32_t w, uint32_t h) {
    (void)data;
    (void)w;
    (void)h;
    zwlr_layer_surface_v1_ack_configure(surface, serial);
}

static void layer_surface_closed(void *data,
                                struct zwlr_layer_surface_v1 *surface) {
    (void)data;
    (void)surface;
    running = 0;
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = layer_surface_configure,
    .closed = layer_surface_closed,
};

static void registry_global(void *data, struct wl_registry *registry,
                           uint32_t name, const char *interface,
                           uint32_t version) {
    (void)data;
    (void)version;
    
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        if (!output) {
            output = wl_registry_bind(registry, name, &wl_output_interface, 1);
        }
    }
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = noop,
};

static struct wl_buffer *create_buffer(cairo_surface_t **cairo_surface) {
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, WIDTH);
    int size = stride * HEIGHT;

    char shm_name[32];
    snprintf(shm_name, sizeof(shm_name), "/dash-%d", getpid());
    
    int fd = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd < 0) {
        perror("shm_open");
        return NULL;
    }
    shm_unlink(shm_name);
    
    if (ftruncate(fd, size) < 0) {
        perror("ftruncate");
        close(fd);
        return NULL;
    }

    void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return NULL;
    }

    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, WIDTH, HEIGHT,
                                                          stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);

    *cairo_surface = cairo_image_surface_create_for_data(data,
        CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT, stride);

    return buffer;
}

static void draw_bar(cairo_surface_t *cairo_surface) {
    cairo_t *cairo = cairo_create(cairo_surface);

    // Clear background
    cairo_set_source_rgba(cairo, BAR_BG_COLOR);
    cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cairo);

    PangoLayout *layout = pango_cairo_create_layout(cairo);
    PangoFontDescription *desc = pango_font_description_from_string("JetBrainsMono Nerd Font 18");
    pango_layout_set_font_description(layout, desc);
    pango_layout_set_text(layout, "TOP TEXT", -1);

    int text_width, text_height;
    pango_layout_get_pixel_size(layout, &text_width, &text_height);

    cairo_set_source_rgba(cairo, BAR_FG_COLOR);
    cairo_move_to(cairo, 0, 0);
    pango_cairo_show_layout(cairo, layout);

    // Draw workspace info on the left
    pango_layout_set_text(layout, "BOTTOM TEXT", -1);
    pango_layout_get_pixel_size(layout, &text_width, &text_height);
    cairo_move_to(cairo, 0, (HEIGHT - text_height));
    pango_cairo_show_layout(cairo, layout);

    pango_font_description_free(desc);
    g_object_unref(layout);
    cairo_destroy(cairo);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "Failed to connect to Wayland display\n");
        return 1;
    }

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);

    if (!compositor || !shm || !layer_shell) {
        fprintf(stderr, "Missing required Wayland interfaces\n");
        return 1;
    }

    surface = wl_compositor_create_surface(compositor);
    layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        layer_shell, surface, output, ZWLR_LAYER_SHELL_V1_LAYER_TOP, "panel");

    zwlr_layer_surface_v1_set_size(layer_surface, WIDTH, HEIGHT);
    zwlr_layer_surface_v1_set_anchor(layer_surface,
        ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, WIDTH);

    zwlr_layer_surface_v1_add_listener(layer_surface, &layer_surface_listener, NULL);

    wl_surface_commit(surface);
    wl_display_roundtrip(display);

    cairo_surface_t *cairo_surface = NULL;
    struct wl_buffer *buffer = create_buffer(&cairo_surface);
    
    time_t last_update = 0;
    while (running) {
        time_t now = time(NULL);
        if (now != last_update) {
            draw_bar(cairo_surface);
            wl_surface_attach(surface, buffer, 0, 0);
            wl_surface_damage(surface, 0, 0, WIDTH, HEIGHT);
            wl_surface_commit(surface);
            last_update = now;
        }

        wl_display_dispatch_pending(display);
        wl_display_flush(display);
        usleep(1000000); // 100ms sleep
    }

    cairo_surface_destroy(cairo_surface);
    wl_buffer_destroy(buffer);
    zwlr_layer_surface_v1_destroy(layer_surface);
    wl_surface_destroy(surface);
    wl_display_disconnect(display);

    return 0;
}
