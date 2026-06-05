#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xft/Xft.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <security/pam_appl.h>
#include <sys/types.h>
#include <pwd.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <poll.h>
#include <stdarg.h>
#include <Imlib2.h>
#include <math.h>
#include "config.h"
#include "blur.h"

static Display *dpy;
static int screen;
static Window root, win;
static GC gc;
static XftFont *xft_font;        /* body / username / password field */
static XftFont *xft_font_time;   /* large clock */
static XftFont *xft_font_date;   /* date line */
static XftFont *xft_font_hint;   /* hint / warning text */
static XftDraw *xft_draw;
static Pixmap bg_pixmap;
static unsigned long text_color_val;

static int failed_attempts = 0;
static int test_mode = 0;
static volatile sig_atomic_t running = 1;

enum SessionState { SESSION_IDLE, SESSION_ACTIVE };
static int session_state = SESSION_IDLE;

enum AuthState { STATE_NORMAL, STATE_WRONG, STATE_CORRECT };
static int auth_state = STATE_NORMAL;

static char *password_ptr;

/* ── PAM conversation ──────────────────────────────────────────────────── */

static int conv(int num_msg, const struct pam_message **msg,
                struct pam_response **resp, void *appdata_ptr) {
    (void)appdata_ptr;
    int i;
    struct pam_response *reply = NULL;

    if (num_msg <= 0 || num_msg > PAM_MAX_NUM_MSG)
        return PAM_CONV_ERR;

    if ((reply = calloc(num_msg, sizeof(struct pam_response))) == NULL)
        return PAM_BUF_ERR;

    for (i = 0; i < num_msg; i++) {
        switch (msg[i]->msg_style) {
            case PAM_PROMPT_ECHO_OFF:
                reply[i].resp = strdup(password_ptr);
                if (!reply[i].resp) goto fail;
                break;
            case PAM_ERROR_MSG:
                fprintf(stderr, "%s\n", msg[i]->msg);
                break;
            case PAM_TEXT_INFO:
                break;
            default:
                goto fail;
        }
    }
    *resp = reply;
    return PAM_SUCCESS;

fail:
    for (i = 0; i < num_msg; i++) {
        if (reply[i].resp) {
            memset(reply[i].resp, 0, strlen(reply[i].resp));
            free(reply[i].resp);
        }
    }
    free(reply);
    return PAM_CONV_ERR;
}

static int verify_password(const char *passwd, const char *user) {
    pam_handle_t *pamh = NULL;
    struct pam_conv pamc = { conv, NULL };
    int ret;

    password_ptr = (char *)passwd;
    ret = pam_start("slock", user, &pamc, &pamh);
    if (ret != PAM_SUCCESS) goto end;
    ret = pam_authenticate(pamh, 0);
    if (ret != PAM_SUCCESS) goto end;
    ret = pam_acct_mgmt(pamh, 0);
end:
    pam_end(pamh, ret);
    return (ret == PAM_SUCCESS) ? 1 : 0;
}

/* ── Resource cleanup ──────────────────────────────────────────────────── */

static void cleanup(void) {
    if (xft_font)      XftFontClose(dpy, xft_font);
    if (xft_font_time) XftFontClose(dpy, xft_font_time);
    if (xft_font_date) XftFontClose(dpy, xft_font_date);
    if (xft_font_hint) XftFontClose(dpy, xft_font_hint);
    if (xft_draw)      XftDrawDestroy(xft_draw);
    if (gc)            XFreeGC(dpy, gc);
    if (bg_pixmap)     XFreePixmap(dpy, bg_pixmap);
    if (win)           XDestroyWindow(dpy, win);
    if (dpy)           XCloseDisplay(dpy);
}

static void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    cleanup();
    exit(1);
}

/* ── Helpers ───────────────────────────────────────────────────────────── */

static double get_luminance(XImage *image) {
    long long r = 0, g = 0, b = 0;
    int count = 0;
    int w = image->width, h = image->height;
    for (int i = 0; i < w * h; i += 10) {
        unsigned long p = XGetPixel(image, i % w, i / w);
        r += (p >> 16) & 0xFF;
        g += (p >>  8) & 0xFF;
        b +=  p        & 0xFF;
        count++;
    }
    if (!count) return 0.0;
    return 0.2126 * (r / count) + 0.7152 * (g / count) + 0.0722 * (b / count);
}

/*
 * Try the configured install path first; fall back to the local
 * assets/ directory so "./slock -t" works from the project root.
 */
static void draw_avatar(const char *path, int x, int y, int size) {
    Imlib_Image image = imlib_load_image(path);
    if (!image)
        image = imlib_load_image(USER_AVATAR_FALLBACK);
    if (!image) return;

    imlib_context_set_image(image);
    int w = imlib_image_get_width();
    int h = imlib_image_get_height();
    int min_dim = (w < h) ? w : h;
    int crop_x  = (w - min_dim) / 2;
    int crop_y  = (h - min_dim) / 2;

    Imlib_Image scaled = imlib_create_cropped_scaled_image(
        crop_x, crop_y, min_dim, min_dim, size, size);
    imlib_free_image();
    if (!scaled) return;

    imlib_context_set_image(scaled);
    imlib_image_set_has_alpha(1);

    DATA32 *data = imlib_image_get_data();
    if (data) {
        int cx   = size / 2;
        int cy   = size / 2;
        int r_sq = (size / 2) * (size / 2);
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                int dx = j - cx, dy = i - cy;
                if (dx * dx + dy * dy > r_sq)
                    data[i * size + j] &= 0x00FFFFFF; /* zero alpha → transparent */
            }
        }
        imlib_image_put_back_data(data);
    }

    imlib_context_set_display(dpy);
    imlib_context_set_visual(DefaultVisual(dpy, screen));
    imlib_context_set_colormap(DefaultColormap(dpy, screen));
    imlib_context_set_drawable(win);
    imlib_context_set_blend(1);
    imlib_render_image_on_drawable(x, y);
    imlib_free_image();
}

/* Helper: allocate an XftColor from a packed 0xRRGGBB value. */
static void make_xft_color(unsigned long rgb, float alpha_f, XftColor *out) {
    unsigned short a = (unsigned short)(alpha_f * 0xffff);
    XRenderColor xrc = {
        .red   = ((rgb >> 16) & 0xFF) * 257,
        .green = ((rgb >>  8) & 0xFF) * 257,
        .blue  = ( rgb        & 0xFF) * 257,
        .alpha = a
    };
    XftColorAllocValue(dpy, DefaultVisual(dpy, screen),
                       DefaultColormap(dpy, screen), &xrc, out);
}

static void free_xft_color(XftColor *c) {
    XftColorFree(dpy, DefaultVisual(dpy, screen),
                 DefaultColormap(dpy, screen), c);
}

/* ── Drawing ───────────────────────────────────────────────────────────── */

static void draw_lock_screen(const char *user, int len) {
    int sw = DisplayWidth(dpy, screen);
    int sh = DisplayHeight(dpy, screen);
    int cx = sw / 2;

    /* 1. Blit cached blurred background */
    XCopyArea(dpy, bg_pixmap, win, gc, 0, 0, sw, sh, 0, 0);

    /*
     * 2. Full-screen tint.
     *    Red  → wrong password
     *    Green → correct password
     *    Values are premultiplied RGBA for XRender PictOpOver.
     *    ~25% opacity: alpha = 0x4000, channel = 0x4000 (full saturation at
     *    this alpha).
     */
    if (auth_state == STATE_WRONG) {
        XRenderColor tint = { 0x4000, 0x0000, 0x0000, 0x4000 };
        XRenderFillRectangle(dpy, PictOpOver, XftDrawPicture(xft_draw),
                             &tint, 0, 0, sw, sh);
    } else if (auth_state == STATE_CORRECT) {
        XRenderColor tint = { 0x0000, 0x4000, 0x0000, 0x4000 };
        XRenderFillRectangle(dpy, PictOpOver, XftDrawPicture(xft_draw),
                             &tint, 0, 0, sw, sh);
    }

    /* 3. Primary text color (white or black depending on background luma) */
    XftColor col;
    make_xft_color(text_color_val, 1.0f, &col);

    /* ── IDLE STATE ─────────────────────────────────────── */
    if (session_state == SESSION_IDLE) {
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        char time_str[32], date_str[64];
        strftime(time_str, sizeof time_str, TIME_FORMAT, tm_info);
        strftime(date_str, sizeof date_str, DATE_FORMAT, tm_info);

        XGlyphInfo ext;
        int time_y = sh * 2 / 5;

        /* Clock */
        XftTextExtentsUtf8(dpy, xft_font_time,
                           (XftChar8 *)time_str, strlen(time_str), &ext);
        XftDrawStringUtf8(xft_draw, &col, xft_font_time,
                          cx - ext.xOff / 2, time_y,
                          (XftChar8 *)time_str, strlen(time_str));

        /* Date */
        XftTextExtentsUtf8(dpy, xft_font_date,
                           (XftChar8 *)date_str, strlen(date_str), &ext);
        XftDrawStringUtf8(xft_draw, &col, xft_font_date,
                          cx - ext.xOff / 2, time_y + 40,
                          (XftChar8 *)date_str, strlen(date_str));

        /* Unlock hint */
        const char *hint = "Click or press a key to unlock";
        XftTextExtentsUtf8(dpy, xft_font_hint,
                           (XftChar8 *)hint, strlen(hint), &ext);
        XftDrawStringUtf8(xft_draw, &col, xft_font_hint,
                          cx - ext.xOff / 2, time_y + 90,
                          (XftChar8 *)hint, strlen(hint));

    /* ── ACTIVE STATE ───────────────────────────────────── */
    } else {
        int avatar_y = sh / 2 - AVATAR_SIZE - 60;
        draw_avatar(USER_AVATAR, cx - AVATAR_SIZE / 2, avatar_y, AVATAR_SIZE);

        /* Username */
        XGlyphInfo ext;
        XftTextExtentsUtf8(dpy, xft_font,
                           (XftChar8 *)user, strlen(user), &ext);
        XftDrawStringUtf8(xft_draw, &col, xft_font,
                          cx - ext.xOff / 2,
                          avatar_y + AVATAR_SIZE + 28,
                          (XftChar8 *)user, strlen(user));

        /* Password field */
        int fx = cx - PASSWD_FIELD_W / 2;
        int fy = avatar_y + AVATAR_SIZE + 50;

        XRenderColor field_bg = { 0x0000, 0x0000, 0x0000, 0x6666 };
        XRenderFillRectangle(dpy, PictOpOver, XftDrawPicture(xft_draw),
                             &field_bg, fx, fy, PASSWD_FIELD_W, PASSWD_FIELD_H);
        XSetForeground(dpy, gc, COLOR_FIELD_BDR);
        XDrawRectangle(dpy, win, gc, fx, fy, PASSWD_FIELD_W, PASSWD_FIELD_H);

        if (auth_state == STATE_CORRECT) {
            /* Green success text inside the field */
            XftColor ok_col;
            make_xft_color(COLOR_SUCCESS, 1.0f, &ok_col);
            const char *ok_msg = "Access Granted";
            XftTextExtentsUtf8(dpy, xft_font,
                               (XftChar8 *)ok_msg, strlen(ok_msg), &ext);
            XftDrawStringUtf8(xft_draw, &ok_col, xft_font,
                              cx - ext.xOff / 2, fy + 22,
                              (XftChar8 *)ok_msg, strlen(ok_msg));
            free_xft_color(&ok_col);

        } else if (len == 0) {
            /* Placeholder */
            XftColor ph_col;
            make_xft_color(COLOR_PH_TEXT, 1.0f, &ph_col);
            XftDrawStringUtf8(xft_draw, &ph_col, xft_font,
                              fx + 10, fy + 22,
                              (XftChar8 *)"Password", 8);
            free_xft_color(&ph_col);

        } else {
            /* Asterisk echo */
            char buf[256];
            memset(buf, '*', len);
            buf[len] = '\0';
            XftDrawStringUtf8(xft_draw, &col, xft_font,
                              fx + 10, fy + 22,
                              (XftChar8 *)buf, len);
        }

        /* Error message + remaining attempts */
        if (auth_state == STATE_WRONG) {
            XftColor err_col;
            make_xft_color(COLOR_FAILURE, 1.0f, &err_col);
            const char *msg = "Incorrect password";
            XftTextExtentsUtf8(dpy, xft_font,
                               (XftChar8 *)msg, strlen(msg), &ext);
            XftDrawStringUtf8(xft_draw, &err_col, xft_font,
                              cx - ext.xOff / 2,
                              fy + PASSWD_FIELD_H + 24,
                              (XftChar8 *)msg, strlen(msg));
            free_xft_color(&err_col);

            /* Remaining attempts with colour coding */
            int remaining = MAX_ATTEMPTS - failed_attempts;
            if (remaining > 0) {
                char warn[64];
                snprintf(warn, sizeof warn, "%d attempt%s remaining",
                         remaining, remaining == 1 ? "" : "s");

                unsigned long warn_rgb;
                if      (remaining <= 2) warn_rgb = COLOR_FAILURE;   /* red    */
                else if (remaining <= 5) warn_rgb = 0xFF8800;        /* orange */
                else                     warn_rgb = 0xFFDD00;        /* yellow */

                XftColor warn_col;
                make_xft_color(warn_rgb, 1.0f, &warn_col);
                XftTextExtentsUtf8(dpy, xft_font_hint,
                                   (XftChar8 *)warn, strlen(warn), &ext);
                XftDrawStringUtf8(xft_draw, &warn_col, xft_font_hint,
                                  cx - ext.xOff / 2,
                                  fy + PASSWD_FIELD_H + 48,
                                  (XftChar8 *)warn, strlen(warn));
                free_xft_color(&warn_col);
            }
        }
    }

    free_xft_color(&col);
}

/* ── X11 initialisation ─────────────────────────────────────────────────── */

static void init_x(void) {
    dpy = XOpenDisplay(NULL);
    if (!dpy) die("Cannot open display\n");

    screen = DefaultScreen(dpy);
    root   = RootWindow(dpy, screen);

    /* Capture and blur BEFORE mapping the window */
    XImage *background = capture_screen(dpy, root);
    if (!background) die("Cannot capture screen\n");
    apply_blur(background, BLUR_RADIUS);
    text_color_val = (get_luminance(background) > 128.0)
                     ? COLOR_TEXT_DARK : COLOR_TEXT_LIGHT;

    XSetWindowAttributes wa;
    wa.override_redirect = 1;
    wa.background_pixel  = COLOR_BACKGROUND;
    win = XCreateWindow(dpy, root,
                        0, 0, DisplayWidth(dpy, screen), DisplayHeight(dpy, screen),
                        0, DefaultDepth(dpy, screen), CopyFromParent,
                        DefaultVisual(dpy, screen),
                        CWOverrideRedirect | CWBackPixel, &wa);

    /* Stash the blurred background as a server-side pixmap */
    bg_pixmap = XCreatePixmap(dpy, win,
                              background->width, background->height,
                              DefaultDepth(dpy, screen));
    gc = XCreateGC(dpy, win, 0, NULL);
    XPutImage(dpy, bg_pixmap, gc, background,
              0, 0, 0, 0, background->width, background->height);
    XDestroyImage(background);

    XSelectInput(dpy, win, KeyPressMask | ExposureMask | ButtonPressMask);
    XMapRaised(dpy, win);

    if (XGrabKeyboard(dpy, root, True,
                      GrabModeAsync, GrabModeAsync, CurrentTime) != GrabSuccess)
        die("Cannot grab keyboard\n");
    if (XGrabPointer(dpy, root, False, ButtonPressMask,
                     GrabModeAsync, GrabModeAsync,
                     None, None, CurrentTime) != GrabSuccess)
        die("Cannot grab pointer\n");

    xft_font      = XftFontOpenName(dpy, screen, FONT ":size=" TOSTRING(FONT_SIZE));
    xft_font_time = XftFontOpenName(dpy, screen, FONT ":size=" TOSTRING(TIME_FONT_SIZE));
    xft_font_date = XftFontOpenName(dpy, screen, FONT ":size=" TOSTRING(DATE_FONT_SIZE));
    xft_font_hint = XftFontOpenName(dpy, screen, FONT ":size=" TOSTRING(HINT_FONT_SIZE));
    if (!xft_font || !xft_font_time || !xft_font_date || !xft_font_hint)
        die("Cannot load fonts\n");

    xft_draw = XftDrawCreate(dpy, win,
                             DefaultVisual(dpy, screen),
                             DefaultColormap(dpy, screen));
}

/* ── Display-manager restart (max-attempts fallback) ───────────────────── */

static void restart_display_manager(void) {
    const char *dm[] = { "/usr/bin/systemctl", "/bin/systemctl", NULL };
    const char *sv[] = { "sddm", "lightdm", "gdm", "xdm", NULL };

    /* Try the generic alias first */
    for (int i = 0; dm[i]; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            char *argv[] = { (char *)dm[i], "is-active", "--quiet",
                             "display-manager.service", NULL };
            char *envp[] = { NULL };
            execve(dm[i], argv, envp);
            exit(1);
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                pid_t rpid = fork();
                if (rpid == 0) {
                    char *argv[] = { (char *)dm[i], "restart",
                                     "display-manager.service", NULL };
                    char *envp[] = { NULL };
                    execve(dm[i], argv, envp);
                    exit(1);
                } else if (rpid > 0) {
                    waitpid(rpid, NULL, 0);
                }
                return;
            }
        }
    }

    /* Fall back to probing specific service names */
    for (int i = 0; dm[i]; i++) {
        for (int j = 0; sv[j]; j++) {
            char service[64];
            snprintf(service, sizeof service, "%s.service", sv[j]);

            pid_t pid = fork();
            if (pid == 0) {
                char *argv[] = { (char *)dm[i], "is-active", "--quiet",
                                 service, NULL };
                char *envp[] = { NULL };
                execve(dm[i], argv, envp);
                exit(1);
            } else if (pid > 0) {
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    pid_t rpid = fork();
                    if (rpid == 0) {
                        char *argv[] = { (char *)dm[i], "restart", service, NULL };
                        char *envp[] = { NULL };
                        execve(dm[i], argv, envp);
                        exit(1);
                    } else if (rpid > 0) {
                        waitpid(rpid, NULL, 0);
                    }
                    return;
                }
            }
        }
    }

    /* Last resort: switch to TTY1 */
    fprintf(stderr, "slock: no display manager found, falling back to TTY1\n");
    pid_t pid = fork();
    if (pid == 0) {
        char *argv[] = { "/usr/bin/chvt", "1", NULL };
        char *envp[] = { NULL };
        execve("/usr/bin/chvt", argv, envp);
        exit(1);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    }
}

/* ── Signal handler ─────────────────────────────────────────────────────── */

static void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

/* ── Entry point ─────────────────────────────────────────────────────────── */

int main(int argc, char **argv) {
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if (argc == 2 && strcmp(argv[1], "-v") == 0) {
        printf("slock-%s\n", VERSION);
        return 0;
    } else if (argc == 2 && strcmp(argv[1], "-t") == 0) {
        test_mode = 1;
    } else if (argc != 1) {
        die("usage: slock [-v|-t]\n");
    }

    struct passwd *pw = getpwuid(getuid());
    if (!pw) die("slock: getpwuid failed\n");
    const char *user = pw->pw_name;

    init_x();

    char passwd[256];
    memset(passwd, 0, sizeof passwd);
    int len = 0;
    XEvent ev;
    KeySym ksym;

    draw_lock_screen(user, len);

    while (running) {
        /*
         * poll() with a 1-second timeout keeps the idle clock live.
         * We must drain Xlib's internal buffer before blocking on the fd —
         * events may be queued without new data arriving on the socket.
         */
        if (!XPending(dpy)) {
            struct pollfd pfd = { .fd = ConnectionNumber(dpy), .events = POLLIN };
            int r = poll(&pfd, 1, 1000);
            if (r < 0 && errno == EINTR) continue;   /* signal, re-check running */
            if (r == 0) {                              /* timeout: tick the clock */
                if (session_state == SESSION_IDLE)
                    draw_lock_screen(user, len);
                continue;
            }
        }

        XNextEvent(dpy, &ev);

        if (ev.type == KeyPress) {
            char buf[32];
            int num = XLookupString(&ev.xkey, buf, sizeof buf, &ksym, 0);

            /* Modifier keys (Shift, Ctrl, Alt…) never do anything */
            if (IsModifierKey(ksym)) continue;

            /*
             * IDLE → ACTIVE transition.
             * Enter/Escape in idle: just activate the login form without
             * attempting auth or adding characters to the buffer.
             * Any other key: activate AND fall through to process it so
             * the first typed character is not lost.
             */
            if (session_state == SESSION_IDLE) {
                session_state = SESSION_ACTIVE;
                if (ksym == XK_Return || ksym == XK_Escape) {
                    draw_lock_screen(user, len);
                    continue;
                }
                /* fall through to process the key in active state */
            }

            /* Clear error badge on first new keypress */
            if (auth_state == STATE_WRONG) auth_state = STATE_NORMAL;

            /* Ignore misc/function/keypad keys in active state */
            if (IsFunctionKey(ksym) || IsMiscFunctionKey(ksym) ||
                IsKeypadKey(ksym)   || IsPFKey(ksym))
                continue;

            if (ksym == XK_Return) {
                passwd[len] = '\0';
                if (verify_password(passwd, user)) {
                    auth_state = STATE_CORRECT;
                    len = 0;
                    memset(passwd, 0, sizeof passwd);
                    draw_lock_screen(user, 0);   /* green tint */
                    sleep(1);
                    running = 0;
                } else {
                    auth_state = STATE_WRONG;
                    failed_attempts++;
                    len = 0;
                    memset(passwd, 0, sizeof passwd);
                    draw_lock_screen(user, 0);   /* red tint + error */
                    if (failed_attempts >= MAX_ATTEMPTS) {
                        sleep(2);                /* let user see the state */
                        running = 0;
                        if (!test_mode) restart_display_manager();
                    }
                }
                continue; /* draw already done above */

            } else if (ksym == XK_Escape) {
                if (test_mode) { running = 0; continue; }
                len = 0;
                memset(passwd, 0, sizeof passwd);

            } else if (ksym == XK_q && test_mode) {
                running = 0;
                continue;

            } else if (ksym == XK_BackSpace || ksym == XK_Delete) {
                if (len > 0) passwd[--len] = '\0';

            } else if (num && !iscntrl((int)buf[0]) &&
                       len < (int)sizeof(passwd) - 1) {
                memcpy(passwd + len, buf, num);
                len += num;
            }

            draw_lock_screen(user, len);

        } else if (ev.type == ButtonPress) {
            if (session_state == SESSION_IDLE) {
                session_state = SESSION_ACTIVE;
                draw_lock_screen(user, len);
            }

        } else if (ev.type == Expose) {
            /* Only redraw on the last expose in a run */
            if (ev.xexpose.count == 0)
                draw_lock_screen(user, len);
        }
    }

    memset(passwd, 0, sizeof passwd);
    int exit_code = (failed_attempts >= MAX_ATTEMPTS && !test_mode) ? 1 : 0;
    cleanup();
    return exit_code;
}
