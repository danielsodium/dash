#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>

#include "overlord.h"

#define SOCKET_PATH "/tmp/passionfruit.sock"

static int ipc_fd = -1;

static int create_socket() {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    
    listen(sock, 1);
    return sock;
}

static int signal_existing_instance() {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    
    const char* msg = "TOGGLE\n";
    write(sock, msg, strlen(msg));
    close(sock);
    return 0;
}

static void cleanup_socket() {
    if (ipc_fd != -1) {
        close(ipc_fd);
        ipc_fd = -1;
    }
    unlink(SOCKET_PATH);
}

static void signal_handler(int sig) {
    (void)sig;
    cleanup_socket();
    exit(0);
}

int main() {
    ipc_fd = create_socket();
    if (ipc_fd < 0) {
        if (signal_existing_instance() == 0) {
            printf("Signaled existing instance\n");
            return 0;
        }
        fprintf(stderr, "Failed to signal existing instance\n");
        return 1;
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    atexit(cleanup_socket);

    overlord_run(ipc_fd);

    close(ipc_fd);
    return 0;
}
