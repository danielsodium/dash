#include "module.h"

#include <unistd.h>

static int open_fd() {
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
        perror("pipe failed");
        return -1;
    }
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return -1;
    }
    if (pid == 0) {
        close(pipe_fds[0]); 
        if (dup2(pipe_fds[1], STDOUT_FILENO) == -1) {
            perror("dup2 failed");
            exit(1);
        }
        close(pipe_fds[1]); 
        execlp("playerctl", "playerctl", "metadata", "--follow", 
               "--format", "{{ title }}\n{{ artist }}", NULL);
        perror("execlp failed");
        exit(1);
    }
    close(pipe_fds[1]);
    return pipe_fds[0];
}

static void init(Module* m) {
    PlayerctlData* d = malloc(sizeof(PlayerctlData));
    m->data = (void*) d;
    d->x = 20;
    m->fds_size = 1;
    m->fds = malloc(sizeof(int));
    m->fds[0] = open_fd();
}

static void draw(Module* m, cairo_t* cairo, PangoLayout* layout) {
    PlayerctlData* d = m->data;
    cairo_move_to(cairo, m->fields[MODULE_X] + d->x, m->fields[MODULE_Y] + d->y);
    cairo_set_source_rgba(cairo, 1.0,1.0,1.0,1.0);
    pango_layout_set_text(layout, d->song, -1);
    pango_cairo_update_layout(cairo, layout);
    pango_cairo_show_layout(cairo, layout);
}

static int callback(Module* m, int fd, PangoLayout* layout) {
    PlayerctlData* p = m->data;
    char buffer[1024];
    ssize_t bytes_read = read(fd, buffer, 127);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        
        if (buffer[bytes_read - 1] == '\n') {
            buffer[bytes_read - 1] = '\0';
        }


        if (buffer[0] == '\0') {
            if (m->fields[MODULE_ACTIVE]) return MODULE_DEACTIVATE | MODULE_UPDATE;
        } else {
            int width_pango, height_pango;
            pango_layout_set_text(layout, buffer, -1);
            pango_layout_get_size(layout, &width_pango, &height_pango);

            m->fields[MODULE_TARGET_W] = width_pango/1024 + p->x * 2;
            p->y = (60 - height_pango/1024)/2;

            strcpy(p->song, buffer);
            if (!m->fields[MODULE_ACTIVE]) return MODULE_ACTIVATE | MODULE_UPDATE;
            return MODULE_UPDATE;
        }
    } 
    else if (bytes_read == -1) {
        perror("read error");
    }
    return 0;
}

void playerctl_module(Module* m) {
    m->init = init;
    m->draw = draw;
    m->callback = callback;
}
