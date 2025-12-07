# Directory structure
SRC_DIR = src
OBJ_DIR = build/bar
BIN_DIR = bin
GEN_DIR = gen
INCLUDE_DIR = include

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g
LIBS = $(shell pkg-config --libs wayland-client cairo pango pangocairo xkbcommon ) -lrt
INCLUDES = $(shell pkg-config --cflags wayland-client cairo pango pangocairo xkbcommon) -I$(INCLUDE_DIR) -I$(GEN_DIR)

# Protocol directories
PROTO_DIR = /usr/share/wayland-protocols
WLR_PROTO = wlr-layer-shell-unstable-v1.xml

# Target and sources
TARGET = bar
DRUN_TARGET = drun
SRC_FILES = main.c wayland.c canvas.c loop.c overlord.c keyboard.c drun.c bar.c status.c widget.c
PROTO_FILES = wlr-layer-shell-unstable-v1-client-protocol.c xdg-shell-client-protocol.c

# Object files from both source and generated directories
SRC_OBJECTS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC_FILES))
PROTO_OBJECTS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(PROTO_FILES))
OBJECTS = $(SRC_OBJECTS) $(PROTO_OBJECTS)

# DRUN object files (separate directory to avoid conflicts)
DRUN_OBJ_DIR = build/drun
DRUN_SRC_OBJECTS = $(patsubst %.c,$(DRUN_OBJ_DIR)/%.o,$(SRC_FILES))
DRUN_PROTO_OBJECTS = $(patsubst %.c,$(DRUN_OBJ_DIR)/%.o,$(PROTO_FILES))
DRUN_OBJECTS = $(DRUN_SRC_OBJECTS) $(DRUN_PROTO_OBJECTS)

# Generated protocol files
XDG_HEADER = $(GEN_DIR)/xdg-shell-client-protocol.h
XDG_SOURCE = $(GEN_DIR)/xdg-shell-client-protocol.c
WLR_HEADER = $(GEN_DIR)/wlr-layer-shell-unstable-v1-client-protocol.h
WLR_SOURCE = $(GEN_DIR)/wlr-layer-shell-unstable-v1-client-protocol.c

all: $(BIN_DIR)/$(TARGET)

# Create necessary directories
$(OBJ_DIR) $(DRUN_OBJ_DIR) $(BIN_DIR) $(GEN_DIR):
	mkdir -p $@

# Generate xdg-shell protocol files
$(XDG_HEADER): | $(GEN_DIR)
	wayland-scanner client-header $(PROTO_DIR)/stable/xdg-shell/xdg-shell.xml $@

$(XDG_SOURCE): $(XDG_HEADER)
	wayland-scanner private-code $(PROTO_DIR)/stable/xdg-shell/xdg-shell.xml $@

# Generate wlr-layer-shell protocol files
$(WLR_HEADER): $(XDG_HEADER)
	@if [ -f "$(WLR_PROTO)" ]; then \
		wayland-scanner client-header $(WLR_PROTO) $@; \
	elif [ -f "/usr/share/wlr-protocols/unstable/$(WLR_PROTO)" ]; then \
		wayland-scanner client-header /usr/share/wlr-protocols/unstable/$(WLR_PROTO) $@; \
	else \
		echo "Error: wlr-layer-shell protocol not found"; \
		echo "Install wlroots or place $(WLR_PROTO) in current directory"; \
		exit 1; \
	fi

$(WLR_SOURCE): $(WLR_HEADER)
	@if [ -f "$(WLR_PROTO)" ]; then \
		wayland-scanner private-code $(WLR_PROTO) $@; \
	elif [ -f "/usr/share/wlr-protocols/unstable/$(WLR_PROTO)" ]; then \
		wayland-scanner private-code /usr/share/wlr-protocols/unstable/$(WLR_PROTO) $@; \
	else \
		echo "Error: wlr-layer-shell protocol not found"; \
		exit 1; \
	fi

# Compile source files from src directory
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(WLR_HEADER) $(XDG_HEADER) | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile generated protocol sources - these must be generated first
$(OBJ_DIR)/xdg-shell-client-protocol.o: $(XDG_SOURCE) $(WLR_HEADER) $(XDG_HEADER) | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/wlr-layer-shell-unstable-v1-client-protocol.o: $(WLR_SOURCE) $(WLR_HEADER) $(XDG_HEADER) | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Link the final binary
$(BIN_DIR)/$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(OBJECTS) $(LIBS) -o $@

# DRUN target rules
drun: $(BIN_DIR)/$(DRUN_TARGET)

# Compile source files for DRUN with -DDRUN flag
$(DRUN_OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(WLR_HEADER) $(XDG_HEADER) | $(DRUN_OBJ_DIR)
	$(CC) $(CFLAGS) -DDRUN $(INCLUDES) -c $< -o $@

# Compile generated protocol sources for DRUN
$(DRUN_OBJ_DIR)/xdg-shell-client-protocol.o: $(XDG_SOURCE) $(WLR_HEADER) $(XDG_HEADER) | $(DRUN_OBJ_DIR)
	$(CC) $(CFLAGS) -DDRUN $(INCLUDES) -c $< -o $@

$(DRUN_OBJ_DIR)/wlr-layer-shell-unstable-v1-client-protocol.o: $(WLR_SOURCE) $(WLR_HEADER) $(XDG_HEADER) | $(DRUN_OBJ_DIR)
	$(CC) $(CFLAGS) -DDRUN $(INCLUDES) -c $< -o $@

# Link the DRUN binary
$(BIN_DIR)/$(DRUN_TARGET): $(DRUN_OBJECTS) | $(BIN_DIR)
	$(CC) $(DRUN_OBJECTS) $(LIBS) -o $@

run:
	./bin/bar

clean:
	rm -rf $(OBJ_DIR) $(DRUN_OBJ_DIR) $(BIN_DIR) $(GEN_DIR)

install: $(BIN_DIR)/$(TARGET)
	install -Dm755 $(BIN_DIR)/$(TARGET) /usr/local/bin/$(TARGET)

uninstall:
	rm -f /usr/local/bin/$(TARGET)

.PHONY: all clean install uninstall drun
