#ifndef UI_H
#define UI_H

#include <X11/Xlib.h>

/**
 * Draws the user interface elements on the screen.
 *
 * @param display The display connection to the X server.
 * @param window The window where the UI should be drawn.
 * @param gc The graphics context used for drawing.
 * @param username The username to be displayed on the lock screen.
 * @param input The current password input from the user.
 * @param input_len The length of the password input.
 * @param failed_attempts The number of failed password attempts.
 */
void draw_ui(Display *display, Window window, GC gc, const char *username, const char *input, int input_len, int failed_attempts);

#endif // UI_H
