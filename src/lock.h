#ifndef LOCK_H
#define LOCK_H

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

// Define the maximum number of password attempts before reboot
#define MAX_ATTEMPTS 10

// Structure to hold lock screen data
typedef struct {
    Display *display;
    Window root;
    Window win;
    int screen;
    Visual *visual;
    Colormap colormap;
    GC gc;
    XftDraw *xftDraw;
    XftFont *font;
    XftColor xftColor;
} LockScreen;

// Function prototypes
void lock_screen(LockScreen *lockScreen, const char *password);
void setup_lock_screen(LockScreen *lockScreen);
void draw_interface(LockScreen *lockScreen, const char *input, const char *message);
int verify_password(const char *input);

#endif // LOCK_H
