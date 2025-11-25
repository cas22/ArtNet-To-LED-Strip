import os
import shutil
import subprocess
from SCons.Script import Import

Import("env")

def copy_firmware(source, target, env):
    firmware_path = str(target[0])
    env_name = env["PIOENV"]  # the environment name
    project_dir = env.subst("$PROJECT_DIR")
    build_dir = env.subst("$BUILD_DIR")

    # Create: build_output/<env_name>/
    destination_dir = os.path.join(project_dir, "builds", env_name)
    os.makedirs(destination_dir, exist_ok=True)

    # Keep original filename (usually firmware.bin)
    dest_path = os.path.join(destination_dir, os.path.basename(firmware_path))

    # Copy firmware
    shutil.copy(firmware_path, dest_path)
    print(f"Copied {firmware_path} → {dest_path}")

    # Also copy LittleFS image. If it's not present, attempt to build it first
    littlefs_src = os.path.join(build_dir, "littlefs.bin")
    if not os.path.exists(littlefs_src):
        print("LittleFS image not found in build dir; attempting to build filesystem image...")
        result = subprocess.run(
            ["pio", "run", "-e", env_name, "-t", "buildfs"],
            cwd=project_dir
        )
        if result.returncode == 0:
            print("Built LittleFS image successfully")
        else:
            print("Warning: Failed to build LittleFS image (buildfs). Will search build dir for existing image.")

    # Try to copy littlefs by known name or by any filename containing 'littlefs'
    if os.path.exists(littlefs_src):
        littlefs_dest = os.path.join(destination_dir, os.path.basename(littlefs_src))
        shutil.copy(littlefs_src, littlefs_dest)
        print(f"Copied {littlefs_src} → {littlefs_dest}")
    else:
        found = False
        for fname in os.listdir(build_dir):
            if "littlefs" in fname and fname.endswith(".bin"):
                littlefs_src = os.path.join(build_dir, fname)
                littlefs_dest = os.path.join(destination_dir, fname)
                shutil.copy(littlefs_src, littlefs_dest)
                print(f"Copied {littlefs_src} → {littlefs_dest}")
                found = True
                break
        if not found:
            print("No LittleFS image found to copy into builds folder.")

# Run after firmware binary is built
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_firmware)