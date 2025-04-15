# TLE Data Updater

This script automatically downloads Two-Line Element (TLE) data from Celestrak every 48 hours and saves it to the `littlefsflash/tle_eph` file.

## Features

- Downloads TLE data from Celestrak's amateur satellite database
- Updates every 48 hours automatically
- Logs all activities to `tle_update.log`
- Records the timestamp of the last update in `littlefsflash/latest_update.txt`
- Runs as a system service that starts automatically on boot

## Requirements

- Python 3.12.3
- Required Python packages:
  - requests
  - schedule

## Installation

1. Install the required Python packages:

```bash
pip install requests schedule
```

2. Make the script executable:

```bash
chmod +x update_tle.py
```

3. Set up the systemd service:

```bash
# Copy the service file to the systemd directory
sudo cp tle-updater.service /etc/systemd/system/

# Reload systemd to recognize the new service
sudo systemctl daemon-reload

# Enable the service to start on boot
sudo systemctl enable tle-updater.service

# Start the service
sudo systemctl start tle-updater.service
```

## Manual Usage

To run the script manually:

```bash
python3 update_tle.py
```

## Checking Service Status

To check if the service is running:

```bash
sudo systemctl status tle-updater.service
```

To view the logs:

```bash
sudo journalctl -u tle-updater.service
```

Or check the log file directly:

```bash
cat tle_update.log
```

## Troubleshooting

If the service fails to start, check the logs for error messages:

```bash
sudo journalctl -u tle-updater.service -n 50
```

Common issues:
- Network connectivity problems
- Permission issues with the output directory
- Missing Python dependencies 