#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include "lock.h"

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void draw_blurred_background(Display *display, Window win, int screen) {
    // A placeholder function to draw a blurred background
    // Currently, this fills the window with a solid color
    GC gc = XCreateGC(display, win, 0, NULL);
    XSetForeground(display, gc, BlackPixel(display, screen));
    XFillRectangle(display, win, gc, 0, 0, DisplayWidth(display, screen), DisplayHeight(display, screen));
    XFreeGC(display, gc);
}

static void draw_text(Display *display, Window win, XftDraw *xftDraw, XftColor *xftColor, XftFont *font, const char *text, int x, int y) {
    XftDrawStringUtf8(xftDraw, xftColor, font, x, y, (XftChar8 *)text, strlen(text));
}

void lock_screen(LockScreen *lockScreen, const char *password) {
    XEvent event;
    char input[256] = {0};
    int inputLength = 0;
    int attempts = 0;

    char message[256] = "Please enter your password...";

    while (attempts < MAX_ATTEMPTS) {
        // Clear the window and redraw the background
        draw_blurred_background(lockScreen->display, lockScreen->win, lockScreen->screen);
        draw_interface(lockScreen, input, message);

        XNextEvent(lockScreen->display, &event);
        if (event.type == KeyPress) {
            char buf[32];
            KeySym keysym;
            int num = XLookupString(&event.xkey, buf, sizeof(buf), &keysym, NULL);

            if (keysym == XK_Return) {
                if (verify_password(input)) {
                    return; // Unlock
                } else {
                    attempts++;
                    snprintf(message, sizeof(message), "Incorrect password. Attempts left: %d", MAX_ATTEMPTS - attempts);
                    memset(input, 0, sizeof(input));
                    inputLength = 0;
                }
            } else if (keysym == XK_BackSpace) {
                if (inputLength > 0) {
                    input[--inputLength] = '\0';
                }
            } else if (!iscntrl((int)buf[0]) && inputLength + num < sizeof(input)) {
                strncpy(input + inputLength, buf, num);
                inputLength += num;
                input[inputLength] = '\0';
            }
        }
    }

    if (attempts >= MAX_ATTEMPTS) {
        // Restart the display manager after too many failed attempts
        system("systemctl restart sddm.service");
    }
}

void setup_lock_screen(LockScreen *lockScreen) {
    lockScreen->display = XOpenDisplay(NULL);
    if (!lockScreen->display) {
        die("Cannot open display");
    }

    lockScreen->screen = DefaultScreen(lockScreen->display);
    lockScreen->root = RootWindow(lockScreen->display, lockScreen->screen);
    lockScreen->visual = DefaultVisual(lockScreen->display, lockScreen->screen);
    lockScreen->colormap = DefaultColormap(lockScreen->display, lockScreen->screen);

    lockScreen->win = XCreateSimpleWindow(lockScreen->display, lockScreen->root, 0, 0,
                                          DisplayWidth(lockScreen->display, lockScreen->screen),
                                          DisplayHeight(lockScreen->display, lockScreen->screen),
                                          0, BlackPixel(lockScreen->display, lockScreen->screen),
                                          BlackPixel(lockScreen->display, lockScreen->screen));
    XSelectInput(lockScreen->display, lockScreen->win, KeyPressMask | KeyReleaseMask | ExposureMask);
    XMapWindow(lockScreen->display, lockScreen->win);

    lockScreen->gc = XCreateGC(lockScreen->display, lockScreen->win, 0, NULL);

    lockScreen->font = XftFontOpenName(lockScreen->display, lockScreen->screen, "monospace:size=12");
    if (!lockScreen->font) {
        die("Cannot open font");
    }

    lockScreen->xftDraw = XftDrawCreate(lockScreen->display, lockScreen->win, lockScreen->visual, lockScreen->colormap);
    if (!lockScreen->xftDraw) {
        die("Cannot create XftDraw");
    }

    XRenderColor renderColor = {0xffff, 0xffff, 0xffff, 0xffff};
    if (!XftColorAllocValue(lockScreen->display, lockScreen->visual, lockScreen->colormap, &renderColor, &lockScreen->xftColor)) {
        die("Cannot allocate XftColor");
    }
}
