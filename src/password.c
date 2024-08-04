#include <stdlib.h>
#include <string.h>
#include <shadow.h>
#include <crypt.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "password.h"

static const char *get_password_hash() {
    struct passwd *pw;
    struct spwd *sp;

    errno = 0;
    if (!(pw = getpwuid(getuid()))) {
        if (errno) {
            fprintf(stderr, "complex_dwm_slock: getpwuid: %s\n", strerror(errno));
        } else {
            fprintf(stderr, "complex_dwm_slock: cannot retrieve password entry\n");
        }
        return NULL;
    }

    if (!(sp = getspnam(pw->pw_name))) {
        fprintf(stderr, "complex_dwm_slock: getspnam: cannot retrieve shadow entry. Make sure to run as root.\n");
        return NULL;
    }

    return sp->sp_pwdp;
}

int verify_password(const char *input) {
    const char *hash = get_password_hash();
    if (!hash) {
        return -1;
    }

    char *encrypted_input = crypt(input, hash);
    if (!encrypted_input) {
        fprintf(stderr, "complex_dwm_slock: crypt: %s\n", strerror(errno));
        return -1;
    }

    return strcmp(encrypted_input, hash) == 0;
}
