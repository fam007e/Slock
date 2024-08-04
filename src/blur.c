#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <stdio.h>
#include "blur.h"

// Simple box blur implementation
void box_blur(XImage *image, int blur_radius) {
    int width = image->width;
    int height = image->height;
    int bpp = image->bits_per_pixel / 8;

    unsigned char *data = (unsigned char *)image->data;
    unsigned char *blurred_data = (unsigned char *)malloc(width * height * bpp);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int r = 0, g = 0, b = 0;
            int count = 0;

            for (int dy = -blur_radius; dy <= blur_radius; ++dy) {
                for (int dx = -blur_radius; dx <= blur_radius; ++dx) {
                    int ix = x + dx;
                    int iy = y + dy;

                    if (ix >= 0 && ix < width && iy >= 0 && iy < height) {
                        unsigned char *pixel = data + ((iy * width + ix) * bpp);
                        r += pixel[2];
                        g += pixel[1];
                        b += pixel[0];
                        ++count;
                    }
                }
            }

            unsigned char *blurred_pixel = blurred_data + ((y * width + x) * bpp);
            blurred_pixel[2] = r / count;
            blurred_pixel[1] = g / count;
            blurred_pixel[0] = b / count;
        }
    }

    memcpy(data, blurred_data, width * height * bpp);
    free(blurred_data);
}

void capture_screen(Display *display, Window root, XImage **image) {
    int screen = DefaultScreen(display);
    int width = DisplayWidth(display, screen);
    int height = DisplayHeight(display, screen);

    *image = XGetImage(display, root, 0, 0, width, height, AllPlanes, ZPixmap);
    if (!*image) {
        fprintf(stderr, "Failed to capture screen image.\n");
        exit(EXIT_FAILURE);
    }
}

void apply_blur(Display *display, Window root, int blur_radius) {
    XImage *image = NULL;
    capture_screen(display, root, &image);

    // Apply the box blur
    box_blur(image, blur_radius);

    // Put the blurred image back to the root window
    GC gc = DefaultGC(display, DefaultScreen(display));
    XPutImage(display, root, gc, image, 0, 0, 0, 0, image->width, image->height);

    XDestroyImage(image);
}
