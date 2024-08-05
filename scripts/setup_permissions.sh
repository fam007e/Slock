#!/usr/bin/bash

# setup_permissions.sh
# This script sets up permissions for complex_dwm_slock

# Exit on any error
set -e

# Define paths and variables
LOCK_BIN="/usr/local/bin/complex_dwm_slock"
LOG_FILE="/var/log/slock.log"
USER="username"
GROUP="wheel"

# Ensure only root can run this script
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

# Install binary and set permissions
echo "Installing complex_dwm_slock binary to $LOCK_BIN"
install -m 755 ./bin/complex_dwm_slock "$LOCK_BIN"

# Set ownership to the specified user and group
echo "Setting ownership of $LOCK_BIN to $USER:$GROUP"
chown $USER:$GROUP "$LOCK_BIN"

# Set the setuid bit to allow slock to run with elevated privileges
echo "Setting setuid bit on $LOCK_BIN"
chmod u+s "$LOCK_BIN"

# Ensure the log file is writable by the user running slock
echo "Setting up log file at $LOG_FILE"
touch "$LOG_FILE"
chown $USER:$GROUP "$LOG_FILE"
chmod 644 "$LOG_FILE"

# Print completion message
echo "Permissions setup complete."

exit 0
