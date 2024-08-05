# Complex DWM Slock

`complex_dwm_slock` is an enhanced screen locker for DWM (Dynamic Window Manager). It provides a visually
 appealing lock screen with features such as screen blur, time display, and user-friendly password prompts.

## Features

- **Blurred Screen Background**: Blurs the screen content for privacy while locked.
- **Time Display**: Shows the current time in bold at the bottom left corner.
- **User Information**: Displays the username and a password prompt.
- **Responsive UI**: Provides feedback on incorrect password attempts.
- **Security**: Automatically reboots after 10 failed attempts, restarts SDDM, and disables all TTYs.

## Installation

### Prerequisites

- X11 development libraries (`libX11`, `libXext`, `libXrandr`)
- C Compiler (GCC or Clang)
- Make
- PAM (Pluggable Authentication Modules) for password handling

### Building from Source

1. **Clone the Repository**
   ```bash
   git clone https://github.com/yourusername/complex_dwm_slock.git
   cd complex_dwm_slock
   ```

2. **Compile the Source Code**
  ```bash
  make
  ```

3. **Install the Binary**
  ```bash
  sudo make install
  ```

4. **Setup Permissions**
  ```bash
  sudo ./scripts/setup_permissions.sh
  ```

## Configuration

Configuration for `complex_dwm_slock` can be adjusted via the `config/slock.conf` file. Edit this file to
change settings like username display, blur level, and maximum password attempts. 
 
  ```ini
  [General] 
  username=yourusername
  max_attempts=10

  [Appearance]
  blur_level=10
  font=Monospace 12
  ```
 
## Usage

To start the screen locker, simply run:

  ```bash
  complex_dwm_slock
  ```

**Command-line Options**
- `-v`: Display version information.

## Security Considerations

- Ensure that the screen locker binary has the correct permissions to run with elevated privileges. This is
  necessary for it to effectively lock the screen and handle system-level events.
- Disable virtual terminal switching and the X server restart key combination in your `xorg.conf` for increased
  security:
  ```bash
  Section "ServerFlags"
    Option "DontVTSwitch" "True"
    Option "DontZap" "True"
  EndSection
  ```

## Customization

- The appearance of the lock screen can be modified by editing the `src/ui.c` file and recompiling.
- Adjust the blur algorithm in src/blur.c to experiment with different blur effects.

## Troubleshooting

- **Screen Does Not Lock**: Ensure all dependencies are installed and the binary has the necessary permissions.
- **Blur Effect Not Visible**: Verify that your system supports the required X extensions (`XRender`).
- **Permission Denied Errors**: Make sure to run `setup_permissions.sh` with root privileges.

## Contributing

Contributions are welcome! Feel free to submit a pull request or open an issue to discuss potential improvements.

## License

This project is licensed under the MIT License. See the [`LICENSE`](LICENSE) file for more details.

## Acknowledgments

I would like to thank the following projects and resources that inspired and aided in the development of `complex_dwm_slock`:

- [slock](https://tools.suckless.org/slock/) - The simple X screen locker that served as the base and inspiration for this project.
- [dwm](https://dwm.suckless.org/) - The dynamic window manager that seamlessly integrates with `slock`.
- [X11](https://www.x.org/wiki/) - The X Window System that provides the underlying graphical environment.
- [MIT License](https://opensource.org/licenses/MIT) - For the open-source licensing model.

Special thanks to the open-source community for their invaluable contributions and support.


### Explanation

- **Introduction**: The README starts with a brief description of the `complex_dwm_slock` project and its main features.
- **Installation**: Provides detailed steps for installing the project from source, including prerequisites and build instructions.
- **Configuration**: Describes how to configure the application via a configuration file.
- **Usage**: Instructions for running the application, with a brief description of command-line options.
- **Security Considerations**: Offers guidance on enhancing security through system settings.
- **Customization**: Tips on how to modify the locker's appearance and behavior.
- **Troubleshooting**: Common issues and solutions.
- **Contributing**: Information on how to contribute to the project.
- **License**: Specifies the licensing terms for the project.
- **Acknowledgments**: Credits the original `slock` and other inspirations.

This README should provide users with a comprehensive understanding of how to install, configure, and use your
`complex_dwm_slock` application.

