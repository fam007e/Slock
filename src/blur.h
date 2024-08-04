#ifndef BLUR_H
#define BLUR_H

#include <X11/Xlib.h>

// Function to apply a blur effect to the screen
void apply_blur(Display *display, Window root, int blur_radius);

#endif // BLUR_H
