import os
import subprocess
from pathlib import Path

Import("env")

def before_upload(source, target, env):
    """Run uploadfs before uploading firmware"""
    print("\n" + "="*60)
    print("Uploading LittleFS filesystem first...")
    print("="*60)
    
    # Get the environment name
    env_name = env.get("PIOENV", "esp32")
    
    # Run uploadfs
    result = subprocess.run(
        ["pio", "run", "-e", env_name, "-t", "uploadfs"],
        cwd=env.get("PROJECT_DIR")
    )
    
    if result.returncode != 0:
        print("ERROR: LittleFS upload failed!")
        raise Exception("uploadfs failed")
    
    print("\n" + "="*60)
    print("LittleFS uploaded successfully. Now uploading firmware...")
    print("="*60 + "\n")

env.AddPreAction("upload", before_upload)