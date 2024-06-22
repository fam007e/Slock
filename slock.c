#define _XOPEN_SOURCE 500
#include <X11/extensions/Xrandr.h>
#include <X11/XF86keysym.h>
#include <Xft/Xft.h>
#include <MagickWand/MagickWand.h>
#include "arg.h"
#include "util.h"

char *argv0;

enum {
    INIT,
    INPUT,
    FAILED,
    NUMCOLS
};

struct lock {
    int screen;
    Window root, win;
    Pixmap pmap;
    unsigned long colors[NUMCOLS];
};

#include "config.h"

static void die(const char *errstr, ...) {
    va_list ap;
    va_start(ap, errstr);
    vfprintf(stderr, errstr, ap);
    va_end(ap);
    exit(1);
}

Pixmap capture_screen(Display *dpy, int screen) {
    // Implement screen capture
}

Pixmap blur_screen(Display *dpy, int screen, Pixmap pixmap) {
    // Implement screen blur using ImageMagick
}

void draw_text(Display *dpy, Window win, const char *text, int x, int y, XftFont *font, XftColor *color) {
    XftDraw *draw = XftDrawCreate(dpy, win, DefaultVisual(dpy, 0), DefaultColormap(dpy, 0));
    XftDrawString8(draw, color, font, x, y, (XftChar8 *)text, strlen(text));
    XftDrawDestroy(draw);
}

void readpw(Display *dpy, struct xrandr *rr, struct lock **locks, int nscreens, const char *hash) {
    XRRScreenChangeNotifyEvent *rre;
    char buf[32], passwd[256], *inputhash;
    int num, screen, running, failure, oldc, attempts = 0;
    unsigned int len, color;
    KeySym ksym;
    XEvent ev;

    len = 0;
    running = 1;
    failure = 0;
    oldc = INIT;

    while (running && !XNextEvent(dpy, &ev)) {
        if (ev.type == KeyPress) {
            explicit_bzero(&buf, sizeof(buf));
            num = XLookupString(&ev.xkey, buf, sizeof(buf), &ksym, 0);
            if (IsKeypadKey(ksym)) {
                if (ksym == XK_KP_Enter)
                    ksym = XK_Return;
                else if (ksym >= XK_KP_0 && ksym <= XK_KP_9)
                    ksym = (ksym - XK_KP_0) + XK_0;
            }
            if (IsFunctionKey(ksym) ||
                IsKeypadKey(ksym) ||
                IsMiscFunctionKey(ksym) ||
                IsPFKey(ksym) ||
                IsPrivateKeypadKey(ksym))
                continue;
            switch (ksym) {
            case XK_Return:
                passwd[len] = '\0';
                errno = 0;
                if (!(inputhash = crypt(passwd, hash)))
                    fprintf(stderr, "slock: crypt: %s\n", strerror(errno));
                else
                    running = !!strcmp(inputhash, hash);
                if (running) {
                    XBell(dpy, 100);
                    failure = 1;
                    attempts++;
                    handle_incorrect_passwords(attempts);
                }
                explicit_bzero(&passwd, sizeof(passwd));
                len = 0;
                break;
            case XK_Escape:
                explicit_bzero(&passwd, sizeof(passwd));
                len = 0;
                break;
            case XK_BackSpace:
                if (len)
                    passwd[--len] = '\0';
                break;
            default:
                if (num && !iscntrl((int)buf[0]) &&
                    (len + num < sizeof(passwd))) {
                    memcpy(passwd + len, buf, num);
                    len += num;
                }
                break;
            }
            color = len ? INPUT : ((failure || failonclear) ? FAILED : INIT);
            if (running && oldc != color) {
                for (screen = 0; screen < nscreens; screen++) {
                    XSetWindowBackground(dpy,
                                         locks[screen]->win,
                                         locks[screen]->colors[color]);
                    XClearWindow(dpy, locks[screen]->win);
                }
                oldc = color;
            }
        } else if (rr->active && ev.type == rr->evbase + RRScreenChangeNotify) {
            rre = (XRRScreenChangeNotifyEvent*)&ev;
            for (screen = 0; screen < nscreens; screen++) {
                if (locks[screen]->win == rre->window) {
                    if (rre->rotation == RR_Rotate_90 ||
                        rre->rotation == RR_Rotate_270)
                        XResizeWindow(dpy, locks[screen]->win,
                                      rre->height, rre->width);
                    else
                        XResizeWindow(dpy, locks[screen]->win,
                                      rre->width, rre->height);
                    XClearWindow(dpy, locks[screen]->win);
                    break;
                }
            }
        } else {
            for (screen = 0; screen < nscreens; screen++)
                XRaiseWindow(dpy, locks[screen]->win);
        }
    }
}

void handle_incorrect_passwords(int attempts) {
    if (attempts >= 8) {
        // Disable TTY access and shut down the computer
        system("shutdown now");
    }
}

int main(int argc, char **argv) {
    struct xrandr rr;
    struct lock **locks;
    struct passwd *pwd;
    struct group *grp;
    uid_t duid;
    gid_t dgid;
    const char *hash;
    Display *dpy;
    int s, nlocks, nscreens;

    ARGBEGIN {
    case 'v':
        puts("slock-"VERSION);
        return 0;
    default:
        usage();
    } ARGEND

    // Validate user and group
    if (!(pwd = getpwnam(user)))
        die("slock: getpwnam %s: %s\n", user, strerror(errno));
    duid = pwd->pw_uid;

    if (!(grp = getgrnam(group)))
        die("slock: getgrnam %s: %s\n", group, strerror(errno));
    dgid = grp->gr_gid;

    // Validate the password hash
    hash = gethash();
    if (!hash)
        die("slock: no passwd entry\n");

    if (!(dpy = XOpenDisplay(NULL)))
        die("slock: cannot open display\n");

    // Initialize and capture screens
    nscreens = ScreenCount(dpy);
    locks = calloc(nscreens, sizeof(struct lock *));
    if (!locks)
        die("slock: calloc: %s\n", strerror(errno));

    for (nlocks = 0, s = 0; s < nscreens; s++) {
        if ((locks[s] = lockscreen(dpy, &rr, s, hash, duid, dgid)) != NULL)
            nlocks++;
        else
            break;
    }

    // Everything is now locked
    XSync(dpy, False);

    // No point in keeping the process around if there are no locks
    if (nlocks == 0) {
        free(locks);
        XCloseDisplay(dpy);
        return 1;
    }

    // Read password and handle unlock
    readpw(dpy, &rr, locks, nscreens, hash);

    // Cleanup
    for (s = 0; s < nscreens; s++)
        if (locks[s] != NULL)
            unlockscreen(dpy, locks[s]);

    free(locks);
    XCloseDisplay(dpy);

    return 0;
}
