# Compiler and flags
CC := gcc
OPTIMIZATIONS := -flto=auto -O2
CFLAGS := -std=c99 -pedantic -Wall -Wno-deprecated-declarations $(OPTIMIZATIONS)
CPPFLAGS := -I/usr/include/freetype2 -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700L
LDFLAGS := -L/usr/X11R6/lib
LDLIBS := -lX11 -lXext -lXrandr -lm -lXft -lfontconfig -lImlib2 -lfreetype -lpam -lXrender

# Directories
SRC_DIR   := src
ASSET_DIR := assets
OBJ_DIR   := obj

# Source and object files
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Output binary
TARGET := slock

# Version: YYYY.MM.DD.<git-short-hash>
CURRENT_DATE  := $(shell date +"%Y.%m.%d")
GIT_HASH      := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
BUILD_VERSION := $(CURRENT_DATE).$(GIT_HASH)
CPPFLAGS += -DBUILD_VERSION=\"$(BUILD_VERSION)\"

# Installation paths
PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin
DATADIR ?= $(PREFIX)/share/slock

# ── Targets ────────────────────────────────────────────────────────────────

.PHONY: all
all: $(TARGET)

# Link
$(TARGET): $(OBJ) | $(OBJ_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# Compile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/config.h | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# Create object directory
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Install binary (setuid) and data assets
.PHONY: install
install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 4755 $(TARGET) $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(DATADIR)
	install -m 644 assets/images/avatar.png $(DESTDIR)$(DATADIR)/avatar.png

# Remove binary and all installed data assets
.PHONY: uninstall
uninstall:
	rm -f  $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -rf $(DESTDIR)$(DATADIR)

# Clean build artefacts
.PHONY: clean
clean:
	rm -rf $(TARGET) $(OBJ_DIR)

# Diagnostic
.PHONY: debug
debug:
	@echo "Source files:  $(SRC)"
	@echo "Object files:  $(OBJ)"
	@echo "Build version: $(BUILD_VERSION)"

.PHONY: print-version
print-version:
	@echo $(BUILD_VERSION)

# Test mode: launch against the running X session (no setuid needed)
.PHONY: test
test: $(TARGET)
	./$(TARGET) -t
