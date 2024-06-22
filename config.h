#ifndef CONFIG_H
#define CONFIG_H
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/extensions/Xrandr.h>
#include <X11/XF86keysym.h>

/* User and group to drop privileges to */
static const char *user = "$USER";
static const char *group = "wheel";

// Function prototypes
void usage();
void lockscreen(Display *dpy, struct xrandr *rr, int screen, const char *hash, uid_t duid, gid_t dgid);
void unlockscreen(Display *dpy, struct lock *lk);
const char *gethash();
int IsKeypadKey(XF86keysym ksym);
int IsFunctionKey(XF86keysym ksym);
int IsMiscFunctionKey(XF86keysym ksym);
int IsPFKey(XF86keysym ksym);
int IsPrivateKeypadKey(XF86keysym ksym);
void explicit_zero(void *s, size_t n);

#endif /* CONFIG_H */
