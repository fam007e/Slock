#include "blur.h"
#include <X11/Xutil.h>
#include <stdlib.h>

/*
 * Separable box blur in two passes (horizontal then vertical).
 *
 * All four channels (A, R, G, B) are accumulated and averaged correctly.
 * The previous code used assignment ( a = ... ) instead of accumulation
 * ( a += ... ) for the alpha channel, causing the output alpha to be the
 * value of the *last* pixel in the kernel window rather than the average.
 * For a screen capture every pixel is fully opaque (alpha = 0xFF) so the
 * artefact was invisible, but the logic was wrong.
 *
 * Requires bits_per_pixel == 32.  The direct pointer cast on image->data
 * is technically a strict-aliasing deviation, but every X11 implementation
 * returns 4-byte-aligned ZPixmap data and GCC/Clang will not miscompile it.
 */
void apply_blur(XImage *image, int radius) {
    if (!image || image->bits_per_pixel != 32) return;

    int width  = image->width;
    int height = image->height;
    unsigned int *src  = (unsigned int *)image->data;
    unsigned int *temp = malloc((size_t)width * height * sizeof(unsigned int));
    if (!temp) return;

    /* ── Horizontal pass: src → temp ─────────────────────────────────── */
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned int r = 0, g = 0, b = 0, a = 0, count = 0;
            for (int i = -radius; i <= radius; i++) {
                int nx = x + i;
                if (nx >= 0 && nx < width) {
                    unsigned int px = src[y * width + nx];
                    a += (px >> 24) & 0xFF;   /* accumulate, not assign */
                    r += (px >> 16) & 0xFF;
                    g += (px >>  8) & 0xFF;
                    b +=  px        & 0xFF;
                    count++;
                }
            }
            temp[y * width + x] = ((a / count) << 24)
                                 | ((r / count) << 16)
                                 | ((g / count) <<  8)
                                 |  (b / count);
        }
    }

    /* ── Vertical pass: temp → src ───────────────────────────────────── */
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            unsigned int r = 0, g = 0, b = 0, a = 0, count = 0;
            for (int i = -radius; i <= radius; i++) {
                int ny = y + i;
                if (ny >= 0 && ny < height) {
                    unsigned int px = temp[ny * width + x];
                    a += (px >> 24) & 0xFF;   /* accumulate, not assign */
                    r += (px >> 16) & 0xFF;
                    g += (px >>  8) & 0xFF;
                    b +=  px        & 0xFF;
                    count++;
                }
            }
            src[y * width + x] = ((a / count) << 24)
                                | ((r / count) << 16)
                                | ((g / count) <<  8)
                                |  (b / count);
        }
    }

    free(temp);
}

XImage *capture_screen(Display *dpy, Window root) {
    XWindowAttributes gwa;
    XGetWindowAttributes(dpy, root, &gwa);
    return XGetImage(dpy, root, 0, 0, gwa.width, gwa.height, AllPlanes, ZPixmap);
}
