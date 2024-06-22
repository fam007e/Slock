# Makefile for slock - simple X display locker

VERSION = 1.4

# Customize these variables
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# X11 path
X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

# Include libraries for X11, Xft (for fonts), and MagickWand (for image processing)
LIBS = -L${X11LIB} -lX11 -lXext -lXrandr -lXft -lMagickWand -L/usr/lib -lMagickWand-7.Q16HDRI -lMagickCore-7.Q16HDRI

# Include flags for X11, Xft, and MagickWand
INCS = -I${X11INC} -I/usr/include/freetype2 -I/usr/include/libpng16 -I/usr/include/pixman-1 -I/usr/include/cairo -I/usr/include/freetype2 -I/usr/include/libpng16 -I/usr/include/pixman-1 -I/usr/include/cairo -I/usr/include/pixman-1 -flto -fno-plt

# C compiler and flags
CC = cc
CFLAGS = -std=c99 -pedantic -Wall -Os ${INCS} ${LIBS} $(shell pkg-config --cflags MagickWand)

# Compiler and linker flags
CPPFLAGS = -DVERSION=\"${VERSION}\" -DXINERAMA

# Installation commands
INSTALL = install
INSTALL_PROGRAM = ${INSTALL} -s -m 755
INSTALL_DATA = ${INSTALL} -m 644

# Directories
SRC = slock.c
OBJ = ${SRC:.c=.o}

# Targets
all: options slock

options:
	@echo slock build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "CPPFLAGS = ${CPPFLAGS}"
	@echo "CC       = ${CC}"

${OBJ}: config.h config.mk

config.h:
	@echo creating $@ from config.def.h
	@cp config.def.h $@

config.mk:
	@echo creating $@ from config.def.mk
	@cp config.def.mk $@

slock: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${CFLAGS} ${CPPFLAGS} ${LIBS}

clean:
	@echo cleaning
	@rm -f slock ${OBJ}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f slock ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/slock
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" < slock.1 > ${DESTDIR}${MANPREFIX}/man1/slock.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/slock.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/slock
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/slock.1

.PHONY: all options clean install uninstall
