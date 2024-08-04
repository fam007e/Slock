# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -Iinclude
LDFLAGS = -lX11 -lXrandr -lXft

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
INCLUDE_DIR = include
ASSETS_DIR = assets
CONFIG_DIR = config

# Source and object files
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

# Target executable
TARGET = $(BIN_DIR)/complex_dwm_slock

# Install directories
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
CONFDIR = /etc/complex_dwm_slock

# Default target
all: $(TARGET)

# Create the target executable
$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up the build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Install the executable and configuration files
install: all
	@mkdir -p $(BINDIR) $(CONFDIR)
	cp -f $(TARGET) $(BINDIR)
	cp -f $(CONFIG_DIR)/* $(CONFDIR)

# Uninstall the executable and configuration files
uninstall:
	rm -f $(BINDIR)/complex_dwm_slock
	rm -rf $(CONFDIR)

# PHONY targets
.PHONY: all clean install uninstall
