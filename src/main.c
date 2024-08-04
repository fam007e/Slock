#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <pwd.h>
#include <shadow.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/Xrandr.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>

#define MAX_ATTEMPTS 10

typedef struct {
    Display *display;
    Window root;
    Window win;
    int screen;
    GC gc;
    XftFont *font;
    XftDraw *xftDraw;
    XftColor xftColor;
    Visual *visual;
    Colormap colormap;
} LockScreen;

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static char *get_password_hash() {
    struct passwd *pw;
    struct spwd *sp;

    if (!(pw = getpwuid(getuid()))) {
        die("Failed to get user info");
    }

    if (!(sp = getspnam(pw->pw_name))) {
        die("Failed to get shadow entry");
    }

    return sp->sp_pwdp;
}

static int verify_password(const char *input) {
    char *hash = get_password_hash();
    char *inputHash = crypt(input, hash);
    return strcmp(hash, inputHash) == 0;
}

static void draw_blurred_background(LockScreen *lockScreen) {
    // Simplified version: fill with a solid color for now
    XSetForeground(lockScreen->display, lockScreen->gc, BlackPixel(lockScreen->display, lockScreen->screen));
    XFillRectangle(lockScreen->display, lockScreen->win, lockScreen->gc, 0, 0, DisplayWidth(lockScreen->display, lockScreen->screen), DisplayHeight(lockScreen->display, lockScreen->screen));
}

static void draw_text(LockScreen *lockScreen, const char *text, int x, int y) {
    XftDrawStringUtf8(lockScreen->xftDraw, &lockScreen->xftColor, lockScreen->font, x, y, (XftChar8 *)text, strlen(text));
}

static void draw_interface(LockScreen *lockScreen, const char *input, const char *message) {
    XClearWindow(lockScreen->display, lockScreen->win);

    // Draw the blurred background
    draw_blurred_background(lockScreen);

    // Draw the current time
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char timeBuffer[64];
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", tm_info);
    draw_text(lockScreen, timeBuffer, 20, 50);

    // Draw the username
    struct passwd *pw = getpwuid(getuid());
    draw_text(lockScreen, pw->pw_name, 20, 100);

    // Draw the password prompt
    char maskedInput[256];
    memset(maskedInput, '*', strlen(input));
    maskedInput[strlen(input)] = '\0';
    char prompt[512];
    snprintf(prompt, sizeof(prompt), "Password: %s", maskedInput);
    draw_text(lockScreen, prompt, 20, 150);

    // Draw the message
    draw_text(lockScreen, message, 20, 200);

    XFlush(lockScreen->display);
}

static void lock_screen(LockScreen *lockScreen) {
    // Set up a simple input mechanism
    char input[256] = {0};
    int inputLength = 0;
    int attempts = 0;

    char message[256] = "Please enter your password...";
    draw_interface(lockScreen, input, message);

    while (attempts < MAX_ATTEMPTS) {
        XEvent event;
        XNextEvent(lockScreen->display, &event);

        if (event.type == KeyPress) {
            char buf[32];
            KeySym keysym;
            int num = XLookupString(&event.xkey, buf, sizeof(buf), &keysym, NULL);

            if (keysym == XK_Return) {
                // Check password
                if (verify_password(input)) {
                    break;
                } else {
                    attempts++;
                    snprintf(message, sizeof(message), "Incorrect password. Attempts left: %d", MAX_ATTEMPTS - attempts);
                    draw_interface(lockScreen, input, message);
                    memset(input, 0, sizeof(input));
                    inputLength = 0;
                }
            } else if (keysym == XK_BackSpace) {
                if (inputLength > 0) {
                    input[--inputLength] = '\0';
                    draw_interface(lockScreen, input, message);
                }
            } else if (!iscntrl((int)buf[0]) && inputLength + num < sizeof(input)) {
                strncpy(input + inputLength, buf, num);
                inputLength += num;
                input[inputLength] = '\0';
                draw_interface(lockScreen, input, message);
            }
        }
    }

    if (attempts >= MAX_ATTEMPTS) {
        system("systemctl restart sddm.service");
    }
}

static void setup_lock_screen(LockScreen *lockScreen) {
    lockScreen->display = XOpenDisplay(NULL);
    if (!lockScreen->display) {
        die("Cannot open display");
    }

    lockScreen->screen = DefaultScreen(lockScreen->display);
    lockScreen->root = RootWindow(lockScreen->display, lockScreen->screen);
    lockScreen->visual = DefaultVisual(lockScreen->display, lockScreen->screen);
    lockScreen->colormap = DefaultColormap(lockScreen->display, lockScreen->screen);

    lockScreen->win = XCreateSimpleWindow(lockScreen->display, lockScreen->root, 0, 0, DisplayWidth(lockScreen->display, lockScreen->screen), DisplayHeight(lockScreen->display, lockScreen->screen), 0, BlackPixel(lockScreen->display, lockScreen->screen), BlackPixel(lockScreen->display, lockScreen->screen));
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

int main() {
    LockScreen lockScreen;
    setup_lock_screen(&lockScreen);
    lock_screen(&lockScreen);

    XftDrawDestroy(lockScreen.xftDraw);
    XftColorFree(lockScreen.display, lockScreen.visual, lockScreen.colormap, &lockScreen.xftColor);
    XftFontClose(lockScreen.display, lockScreen.font);
    XFreeGC(lockScreen.display, lockScreen.gc);
    XDestroyWindow(lockScreen.display, lockScreen.win);
    XCloseDisplay(lockScreen.display);

    return 0;
}
