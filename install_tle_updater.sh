#!/bin/bash

# TLE Updater Installation Script

echo "Installing TLE Updater..."

# Check if Python is installed
if ! command -v python3 &> /dev/null; then
    echo "Python 3 is not installed. Please install Python 3 and try again."
    exit 1
fi

# Install required Python packages using apt
echo "Installing required Python packages using apt..."
apt update
apt install -y python3-requests python3-schedule

# Make the script executable
echo "Making the update script executable..."
chmod +x update_tle.py

# Create the output directory if it doesn't exist
echo "Creating output directory..."
mkdir -p littlefsflash

# Install the systemd service
echo "Installing systemd service..."
if [ -d "/etc/systemd/system" ]; then
    # Copy the service file to the systemd directory
    cp tle-updater.service /etc/systemd/system/
    
    # Reload systemd to recognize the new service
    systemctl daemon-reload
    
    # Enable the service to start on boot
    systemctl enable tle-updater.service
    
    # Start the service
    systemctl start tle-updater.service
    
    echo "Service installed and started successfully."
    echo "You can check the status with: systemctl status tle-updater.service"
else
    echo "Systemd not found. The service will not be installed automatically."
    echo "You can run the script manually with: python3 update_tle.py"
fi

echo "Installation complete!"
echo "See TLE_UPDATER_README.md for more information." 