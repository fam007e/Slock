# config.mk

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Wno-unused-parameter -std=c99 -O2

# X11 libraries
X11LIBS = -lX11 -lXext -lXft
IMAGEMAGICKLIBS = -L/usr/lib -lMagickWand-7.Q16HDRI -lMagickCore-7.Q16HDRI

# Output
BIN = slock
MAN = slock.1

# Installation directories
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# Include directories
INCS = -I/usr/X11R6/include
LIBS = $(X11LIBS) $(IMAGEMAGICKLIBS)

# Build target
all: $(BIN)

$(BIN): slock.o util.o
	$(CC) -o $@ slock.o util.o $(LIBS)

slock.o: slock.c arg.h util.h config.h
	$(CC) $(CFLAGS) $(INCS) -c slock.c

util.o: util.c util.h
	$(CC) $(CFLAGS) $(INCS) -c util.c

clean:
	rm -f $(BIN) slock.o util.o

install: all
	mkdir -p $(PREFIX)/bin
	cp -f $(BIN) $(PREFIX)/bin
	chmod 755 $(PREFIX)/bin/$(BIN)
	mkdir -p $(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" < $(MAN) > $(MANPREFIX)/man1/$(MAN)

uninstall:
	rm -f $(PREFIX)/bin/$(BIN)
	rm -f $(MANPREFIX)/man1/$(MAN)

.PHONY: all clean install uninstall
