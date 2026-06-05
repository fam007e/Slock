#ifndef CONFIG_H
#define CONFIG_H

/* ── Stringification helpers ────────────────────────────────────────────── */
#define STRINGIFY(x) #x
#define TOSTRING(x)  STRINGIFY(x)

/* ── Version ────────────────────────────────────────────────────────────── */
#ifndef BUILD_VERSION
#define BUILD_VERSION unknown
#endif
#define VERSION TOSTRING(BUILD_VERSION)

/* ── User / asset paths ─────────────────────────────────────────────────── */
/*
 * USER_AVATAR       : installed path (used after `make install`)
 * USER_AVATAR_FALLBACK : local path tried when the installed one is missing
 *                        (allows `./slock -t` from the project root)
 */
#define USER_AVATAR          "/usr/local/share/slock/avatar.png"
#define USER_AVATAR_FALLBACK "assets/images/avatar.png"
#define AVATAR_SIZE          150

/* ── Auth limits ────────────────────────────────────────────────────────── */
#define MAX_ATTEMPTS 10
#define BLUR_RADIUS  20

/* ── Fonts ──────────────────────────────────────────────────────────────── */
/*
 * FONT accepts any fontconfig name.
 * Use an absolute path for a custom TTF: "/usr/local/share/slock/font.ttf"
 */
#define FONT           "sans-serif"
#define FONT_SIZE      12    /* body / username / password field */
#define TIME_FONT_SIZE 64    /* large clock */
#define DATE_FONT_SIZE 18    /* date line below clock */
#define HINT_FONT_SIZE 11    /* unlock hint / warning text */

/* ── Time / date formats ────────────────────────────────────────────────── */
#define TIME_FORMAT "%H:%M"
#define DATE_FORMAT "%A %B %d"

/* ── Password field geometry ─────────────────────────────────────────────── */
#define PASSWD_FIELD_W 280
#define PASSWD_FIELD_H 34

/* ── Layout ──────────────────────────────────────────────────────────────── */
#define LAYOUT_MARGIN 50

/* ── Colors (0xRRGGBB, used as X11 pixel values unless noted) ─────────────
 *
 *   COLOR_FIELD_BDR : used with XSetForeground / XDrawRectangle (core X11).
 *                     Must be a plain 24-bit RGB pixel — no alpha byte.
 *   COLOR_PH_TEXT   : placeholder text; unpacked to XRenderColor in code.
 *   COLOR_FIELD_BG  : defined for reference only; the field background is
 *                     drawn with a hard-coded XRenderColor for alpha support.
 * ──────────────────────────────────────────────────────────────────────── */
#define COLOR_BACKGROUND 0x000000   /* window fill before bg_pixmap is ready */
#define COLOR_TEXT_LIGHT 0xFFFFFF   /* text on dark background               */
#define COLOR_TEXT_DARK  0x000000   /* text on light background              */
#define COLOR_FAILURE    0xFF2222   /* wrong-password red                    */
#define COLOR_SUCCESS    0x22FF22   /* correct-password green                */
#define COLOR_FIELD_BDR  0xFFFFFF   /* password field border (white)         */
#define COLOR_PH_TEXT    0x888888   /* placeholder "Password" text           */

#endif /* CONFIG_H */
