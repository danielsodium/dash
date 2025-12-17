TARGET   = pf
SRC_DIR  = src
OBJ_DIR  = build
BIN_DIR  = bin
GEN_DIR  = gen

CC       = gcc
CFLAGS   = -Wall -Wextra -g -Iinclude -I$(GEN_DIR) $(shell pkg-config --cflags wayland-client cairo pango pangocairo xkbcommon)
LIBS     = $(shell pkg-config --libs wayland-client cairo pango pangocairo xkbcommon) -lrt

SRCS     = $(shell find $(SRC_DIR) -name '*.c')
OBJS     = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

PROTO_DIR = /usr/share/wayland-protocols

WLR_LOCAL = wlr-layer-shell-unstable-v1.xml
WLR_SYS   = /usr/share/wlr-protocols/unstable/wlr-layer-shell-unstable-v1.xml
WLR_PROTO = $(if $(wildcard $(WLR_LOCAL)),$(WLR_LOCAL),$(WLR_SYS))
GEN_SRCS = $(GEN_DIR)/xdg-shell-client-protocol.c $(GEN_DIR)/wlr-layer-shell-unstable-v1-client-protocol.c
GEN_OBJS = $(GEN_SRCS:$(GEN_DIR)/%.c=$(OBJ_DIR)/%.o)
GEN_HDRS = $(GEN_SRCS:.c=.h)

all: $(BIN_DIR)/$(TARGET)

$(BIN_DIR)/$(TARGET): $(OBJS) $(GEN_OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) $(GEN_OBJS) $(LIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(GEN_HDRS) | $(OBJ_DIR)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(GEN_DIR)/%.c $(GEN_HDRS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(GEN_DIR)/xdg-shell-client-protocol.h: | $(GEN_DIR)
	wayland-scanner client-header $(PROTO_DIR)/stable/xdg-shell/xdg-shell.xml $@

$(GEN_DIR)/xdg-shell-client-protocol.c: | $(GEN_DIR)
	wayland-scanner private-code $(PROTO_DIR)/stable/xdg-shell/xdg-shell.xml $@

$(GEN_DIR)/wlr-layer-shell-unstable-v1-client-protocol.h: | $(GEN_DIR)
	wayland-scanner client-header $(WLR_PROTO) $@

$(GEN_DIR)/wlr-layer-shell-unstable-v1-client-protocol.c: | $(GEN_DIR)
	wayland-scanner private-code $(WLR_PROTO) $@

$(OBJ_DIR) $(BIN_DIR) $(GEN_DIR):
	mkdir -p $@

run: all
	./$(BIN_DIR)/$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(GEN_DIR)

.PHONY: all run clean
