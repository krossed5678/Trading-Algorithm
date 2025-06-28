#!/usr/bin/env python3
"""
Auto-updater for Trading Algorithm
Checks GitHub releases for updates and safely replaces the executable
"""

import os
import sys
import time
import requests
import shutil
import subprocess
import json
from pathlib import Path

# Configuration
REPO_OWNER = "krossed5678"
REPO_NAME = "Trading-Algorithm"
EXE_NAME = "trading_bot.exe"
GUI_EXE_NAME = "trading_bot_gui.exe"
BENCHMARK_EXE_NAME = "benchmark_performance.exe"
VERSION_FILE = "version.txt"
BACKUP_SUFFIX = ".backup"

def get_local_version():
    """Get the local version from version.txt"""
    version_file = Path(VERSION_FILE)
    if version_file.exists():
        try:
            with open(version_file, 'r') as f:
                return f.read().strip()
        except Exception as e:
            print(f"Error reading version file: {e}")
    return "0.0.0"

def get_github_latest_release():
    """Get the latest release info from GitHub"""
    try:
        url = f"https://api.github.com/repos/{REPO_OWNER}/{REPO_NAME}/releases/latest"
        response = requests.get(url, timeout=10)
        if response.status_code == 200:
            return response.json()
        else:
            print(f"Failed to get release info: {response.status_code}")
            return None
    except Exception as e:
        print(f"Error checking GitHub releases: {e}")
        return None

def download_file(url, filename):
    """Download a file from URL"""
    try:
        print(f"Downloading {filename}...")
        response = requests.get(url, stream=True, timeout=30)
        if response.status_code == 200:
            with open(filename, 'wb') as f:
                shutil.copyfileobj(response.raw, f)
            print(f"Downloaded {filename}")
            return True
        else:
            print(f"Failed to download {filename}: {response.status_code}")
            return False
    except Exception as e:
        print(f"Error downloading {filename}: {e}")
        return False

def is_process_running(exe_name):
    """Check if a process is running"""
    try:
        result = subprocess.run(['tasklist', '/FI', f'IMAGENAME eq {exe_name}'], 
                              capture_output=True, text=True, shell=True)
        return exe_name in result.stdout
    except Exception:
        return False

def wait_for_process_to_close(exe_name, max_wait=30):
    """Wait for a process to close"""
    print(f"Waiting for {exe_name} to close...")
    for i in range(max_wait):
        if not is_process_running(exe_name):
            print(f"{exe_name} closed")
            return True
        time.sleep(1)
        if i % 5 == 0:
            print(f"Still waiting... ({i+1}/{max_wait}s)")
    return False

def backup_file(filename):
    """Create a backup of a file"""
    if os.path.exists(filename):
        backup_name = filename + BACKUP_SUFFIX
        try:
            shutil.copy2(filename, backup_name)
            print(f"Backed up {filename} to {backup_name}")
            return True
        except Exception as e:
            print(f"Failed to backup {filename}: {e}")
            return False
    return True

def restore_backup(filename):
    """Restore a file from backup"""
    backup_name = filename + BACKUP_SUFFIX
    if os.path.exists(backup_name):
        try:
            shutil.copy2(backup_name, filename)
            print(f"Restored {filename} from backup")
            return True
        except Exception as e:
            print(f"Failed to restore {filename}: {e}")
            return False
    return False

def update_executable(exe_name, download_url):
    """Update a single executable"""
    print(f"\nUpdating {exe_name}...")
    
    # Check if process is running
    if is_process_running(exe_name):
        print(f"{exe_name} is currently running.")
        if not wait_for_process_to_close(exe_name):
            print(f"Could not close {exe_name}. Update cancelled.")
            return False
    
    # Create backup
    if not backup_file(exe_name):
        print(f"Failed to backup {exe_name}. Update cancelled.")
        return False
    
    # Download new version
    temp_name = exe_name + ".new"
    if not download_file(download_url, temp_name):
        print(f"Failed to download {exe_name}. Update cancelled.")
        return False
    
    # Replace old file
    try:
        if os.path.exists(exe_name):
            os.remove(exe_name)
        os.rename(temp_name, exe_name)
        print(f"Successfully updated {exe_name}")
        return True
    except Exception as e:
        print(f"Failed to replace {exe_name}: {e}")
        restore_backup(exe_name)
        return False

def main():
    """Main update process"""
    print("=== Trading Algorithm Auto-Updater ===")
    
    # Get current version
    local_version = get_local_version()
    print(f"Current version: {local_version}")
    
    # Check for updates
    print("Checking for updates...")
    release_info = get_github_latest_release()
    if not release_info:
        print("Could not check for updates. Check your internet connection.")
        return False
    
    latest_version = release_info.get('tag_name', '0.0.0')
    print(f"Latest version: {latest_version}")
    
    if latest_version == local_version:
        print("You have the latest version!")
        return True
    
    print(f"Update available: {local_version} -> {latest_version}")
    
    # Get download URLs from release assets
    assets = release_info.get('assets', [])
    download_urls = {}
    
    for asset in assets:
        name = asset.get('name', '')
        if name == EXE_NAME:
            download_urls[EXE_NAME] = asset.get('browser_download_url')
        elif name == GUI_EXE_NAME:
            download_urls[GUI_EXE_NAME] = asset.get('browser_download_url')
        elif name == BENCHMARK_EXE_NAME:
            download_urls[BENCHMARK_EXE_NAME] = asset.get('browser_download_url')
    
    if not download_urls:
        print("No executable files found in the latest release.")
        return False
    
    # Update available executables
    success_count = 0
    for exe_name, url in download_urls.items():
        if update_executable(exe_name, url):
            success_count += 1
    
    # Update version file
    if success_count > 0:
        try:
            with open(VERSION_FILE, 'w') as f:
                f.write(latest_version)
            print(f"Updated version file to {latest_version}")
        except Exception as e:
            print(f"Failed to update version file: {e}")
    
    print(f"\nUpdate complete! {success_count} files updated.")
    
    # Ask if user wants to restart the application
    if success_count > 0:
        response = input("\nWould you like to start the updated application? (y/n): ")
        if response.lower() in ['y', 'yes']:
            if os.path.exists(GUI_EXE_NAME):
                print(f"Starting {GUI_EXE_NAME}...")
                subprocess.Popen([GUI_EXE_NAME])
            elif os.path.exists(EXE_NAME):
                print(f"Starting {EXE_NAME}...")
                subprocess.Popen([EXE_NAME])
    
    return success_count > 0

if __name__ == "__main__":
    try:
        success = main()
        if not success:
            sys.exit(1)
    except KeyboardInterrupt:
        print("\nUpdate cancelled by user.")
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected error: {e}")
        sys.exit(1) 