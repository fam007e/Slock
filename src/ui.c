#include "ui.h"
#include <X11/Xft/Xft.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

static void draw_text(Display *display, Window window, XftDraw *xft_draw, XftColor *color, XftFont *font, int x, int y, const char *text) {
    XftDrawStringUtf8(xft_draw, color, font, x, y, (XftChar8 *)text, strlen(text));
}

void draw_ui(Display *display, Window window, GC gc, const char *username, const char *input, int input_len, int failed_attempts) {
    int screen = DefaultScreen(display);
    int width = DisplayWidth(display, screen);
    int height = DisplayHeight(display, screen);

    // Set up Xft for font rendering
    XftFont *font = XftFontOpenName(display, screen, "Sans-16:bold");
    XftFont *time_font = XftFontOpenName(display, screen, "Sans-24:bold");
    if (!font || !time_font) {
        fprintf(stderr, "Error: Could not load fonts\n");
        return;
    }

    XftDraw *xft_draw = XftDrawCreate(display, window, DefaultVisual(display, screen), DefaultColormap(display, screen));
    XftColor color;
    XRenderColor render_color = {0xffff, 0xffff, 0xffff, 0xffff}; // white color
    XftColorAllocValue(display, DefaultVisual(display, screen), DefaultColormap(display, screen), &render_color, &color);

    // Clear the window
    XClearWindow(display, window);

    // Draw the time in the bottom left corner
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);
    draw_text(display, window, xft_draw, &color, time_font, 20, height - 50, time_buf);

    // Draw the username
    draw_text(display, window, xft_draw, &color, font, width / 2 - 100, height / 2 - 40, username);

    // Draw the password prompt
    draw_text(display, window, xft_draw, &color, font, width / 2 - 100, height / 2, "Password:");

    // Draw the input
    char masked_input[256];
    memset(masked_input, '*', input_len);
    masked_input[input_len] = '\0';
    draw_text(display, window, xft_draw, &color, font, width / 2, height / 2, masked_input);

    // Handle incorrect password input
    if (failed_attempts > 0) {
        draw_text(display, window, xft_draw, &color, font, width / 2 - 100, height / 2 + 40, "Please insert the correct user password here...");
    } else {
        draw_text(display, window, xft_draw, &color, font, width / 2 - 100, height / 2 + 40, "Please insert the user password here...");
    }

    // Clean up
    XftDrawDestroy(xft_draw);
    XftColorFree(display, DefaultVisual(display, screen), DefaultColormap(display, screen), &color);
    XftFontClose(display, font);
    XftFontClose(display, time_font);

    // Flush changes to the window
    XFlush(display);
}
