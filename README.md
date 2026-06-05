# Complex DWM Slock

A secure, visually polished screen locker for DWM and other X11 environments.
It blurs the live desktop, shows a live clock, and uses PAM for authentication.

---

## Features

- **Blurred background** — captures the desktop before mapping the lock window,
  blurs it with a separable box filter, and caches the result as a server-side
  pixmap. No re-blurring on every keypress.
- **Two-state UI**
  - *Idle* — large clock, date, and "Click or press a key to unlock" hint.
    The clock updates every second.
  - *Active* — circular avatar, username, and a password input field.
    Activated by any keypress or mouse click.
- **Visual auth feedback**
  - Wrong password: full-screen red tint + "Incorrect password" message +
    colour-coded remaining-attempts counter (yellow → orange → red).
  - Correct password: full-screen green tint + "Access Granted" for one second.
- **PAM authentication** — uses the system PAM stack; credentials are zeroed
  from memory immediately after verification.
- **Security hardening**
  - `XGrabKeyboard` + `XGrabPointer` prevent other X clients from stealing input.
  - `getpwuid(getuid())` used instead of `getenv("USER")` — immune to
    environment spoofing in a setuid binary.
  - `execve()` with an empty environment used everywhere instead of `system()`.
  - Signal handler sets only a `volatile sig_atomic_t` flag (async-signal-safe).
- **Brute-force fallback** — after `MAX_ATTEMPTS` failures the display manager
  is restarted via `systemctl` (probes `display-manager.service` then specific
  service names). Falls back to `chvt 1` if no display manager is found.
- **Automatic versioning** — build date and Git short hash embedded in the
  binary (`slock -v`).

---

## Prerequisites

### Runtime libraries

| Library | Package (Arch) |
|---------|----------------|
| libX11, libXext, libXrandr | `libx11` `libxext` `libxrandr` |
| libXrender | `libxrender` |
| libXft, libfontconfig | `libxft` `fontconfig` |
| Imlib2 | `imlib2` |
| libpam | `pam` |

### Build tools

- GCC or Clang
- GNU Make
- Git (for version embedding; falls back to `unknown` if absent)

### PAM service file — **required before first use**

`slock` authenticates through the PAM service named `slock`.
Without `/etc/pam.d/slock` every password attempt will fail.

```bash
sudo tee /etc/pam.d/slock << 'PAM'
auth     include  system-auth
account  include  system-auth
PAM
```

On systems without `system-auth` (Debian/Ubuntu), use `common-auth` /
`common-account` instead.

---

## Installation

```bash
git clone https://github.com/fam007e/Complex_DWM_SLock.git
cd Complex_DWM_SLock

make
sudo make install          # installs to /usr/local/bin and /usr/local/share/slock
```

The binary is installed setuid root (`-m 4755`) so PAM can authenticate
against the shadow password database.

The avatar image is installed to `$(DATADIR)/avatar.png`
(`/usr/local/share/slock/avatar.png` by default).

### Custom prefix

```bash
sudo make PREFIX=/opt/slock install
```

### Uninstall

```bash
sudo make uninstall        # removes binary and /usr/local/share/slock/
```

### AUR (Arch Linux)

```bash
yay -S complex-dwm-slock-git
# or
paru -S complex-dwm-slock-git
```

[![AUR package](https://img.shields.io/aur/version/complex-dwm-slock-git?logo=arch-linux)](https://aur.archlinux.org/packages/complex-dwm-slock-git)

---

## Usage

```bash
slock              # lock the screen
slock -v           # print version (YYYY.MM.DD.<git-hash>)
slock -t           # test mode (see below)
```

### Test mode (`-t`)

Runs the locker against the live X session without setuid.
- PAM authentication is still active — the correct user password unlocks it.
- `Esc` or `q` exits without restarting the display manager.
- The avatar is loaded from `assets/images/avatar.png` if the installed path
  (`/usr/local/share/slock/avatar.png`) does not exist, so it works from the
  project root before installation.

```bash
make test
# or
./slock -t
```

---

## Configuration

All configuration is compile-time. Edit `src/config.h` then rebuild.

```bash
make clean && make && sudo make install
```

### Key settings

| Define | Default | Description |
|--------|---------|-------------|
| `USER_AVATAR` | `/usr/local/share/slock/avatar.png` | Installed avatar path |
| `USER_AVATAR_FALLBACK` | `assets/images/avatar.png` | Local fallback for `-t` |
| `AVATAR_SIZE` | `150` | Avatar diameter in pixels |
| `MAX_ATTEMPTS` | `10` | Failed attempts before DM restart |
| `BLUR_RADIUS` | `20` | Box-blur kernel radius |
| `FONT` | `sans-serif` | Fontconfig name or absolute TTF path |
| `TIME_FONT_SIZE` | `64` | Clock font size |
| `DATE_FONT_SIZE` | `18` | Date line font size |
| `HINT_FONT_SIZE` | `11` | Hint / warning font size |
| `TIME_FORMAT` | `%H:%M` | `strftime` format for the clock |
| `DATE_FORMAT` | `%A %B %d` | `strftime` format for the date |
| `PASSWD_FIELD_W` | `280` | Password field width (px) |
| `PASSWD_FIELD_H` | `34` | Password field height (px) |

### Custom font

To use a specific TTF instead of a system font, install the file and set:

```c
#define FONT "/usr/local/share/slock/font.ttf"
```

Update the Makefile `install` target to deploy the font alongside the avatar.

---

## Security considerations

- **Setuid root** is required. The binary has no network access and no shell
  invocations (`system()` is not used anywhere).
- **VT switching** (`Ctrl+Alt+Fn`) can bypass the locker if not disabled.
  Add the following to your Xorg configuration to prevent it:

  ```
  # /etc/X11/xorg.conf.d/90-nodonttwitch.conf
  Section "ServerFlags"
      Option "DontVTSwitch" "true"
  EndSection
  ```

- **Ctrl+Alt+Backspace** (Xorg kill) should also be disabled:

  ```
  Section "ServerFlags"
      Option "DontZap" "true"
  EndSection
  ```

---

## Contributing

Pull requests and issues are welcome.

## License

[MIT License](LICENSE)

## Acknowledgments

Built on the foundations of the suckless `slock` and `dwm` projects and the
broader X11 open-source ecosystem.
