#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
TLE Update Script
This script downloads TLE data from Celestrak every 48 hours
and saves it to the littlefsflash/tle_eph file.
"""

import os
import sys
import time
import logging
import requests
import schedule
import argparse
from datetime import datetime
from pathlib import Path

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler("tle_update.log"),
        logging.StreamHandler(sys.stdout)
    ]
)
logger = logging.getLogger("TLE_Updater")

# Constants
TLE_URL = "https://celestrak.org/NORAD/elements/gp.php?GROUP=amateur&FORMAT=tle"
OUTPUT_FILE = "littlefsflash/tle_eph"
UPDATE_INTERVAL_HOURS = 48

def download_tle():
    """Download TLE data from Celestrak and save it to the output file."""
    try:
        logger.info(f"Downloading TLE data from {TLE_URL}")
        response = requests.get(TLE_URL, timeout=30)
        response.raise_for_status()  # Raise an exception for HTTP errors
        
        # Create directory if it doesn't exist
        output_path = Path(OUTPUT_FILE)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        # Get current timestamp
        current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        # Write the TLE data to the file with timestamp
        with open(OUTPUT_FILE, 'w') as f:
            f.write(response.text)
            f.write(f"\n\n{current_time}\n")
        
        # Log the timestamp of the update
        with open("littlefsflash/latest_update.txt", 'w') as f:
            f.write(current_time)
        
        logger.info(f"TLE data successfully saved to {OUTPUT_FILE}")
        return True
    except Exception as e:
        logger.error(f"Error downloading TLE data: {e}")
        return False

def main():
    """Main function to set up the scheduler and run the initial download."""
    parser = argparse.ArgumentParser(description='TLE Data Updater')
    parser.add_argument('--once', action='store_true', help='Run once and exit')
    args = parser.parse_args()
    
    logger.info("Starting TLE update service")
    
    # Perform initial download
    download_tle()
    
    if args.once:
        logger.info("Running in single-shot mode, exiting after download")
        return
    
    # Schedule regular updates
    schedule.every(UPDATE_INTERVAL_HOURS).hours.do(download_tle)
    
    # Keep the script running
    while True:
        schedule.run_pending()
        time.sleep(60)  # Check every minute

if __name__ == "__main__":
    main() 