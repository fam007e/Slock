#include "util.h"

void die(const char *errstr, ...) {
    va_list ap;
    va_start(ap, errstr);
    vfprintf(stderr, errstr, ap);
    va_end(ap);
    exit(1);
}

char *gethash(void) {
    struct passwd *pw;
    struct spwd *spw;

    if (!(pw = getpwuid(getuid()))) {
        fprintf(stderr, "slock: getpwuid: %s\n", strerror(errno));
        return NULL;
    }

    if (!(spw = getspnam(pw->pw_name))) {
        fprintf(stderr, "slock: getspnam: %s\n", strerror(errno));
        return NULL;
    }

    return spw->sp_pwdp;
}
