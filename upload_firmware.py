import os
import shutil
from SCons.Script import Import

Import("env")

def copy_firmware(source, target, env):
    firmware_path = str(target[0])
    env_name = env["PIOENV"]  # the environment name
    project_dir = env.subst("$PROJECT_DIR")

    # Create: build_output/<env_name>/
    destination_dir = os.path.join(project_dir, "builds", env_name)
    os.makedirs(destination_dir, exist_ok=True)

    # Keep original filename (usually firmware.bin)
    dest_path = os.path.join(destination_dir, os.path.basename(firmware_path))

    # Copy firmware
    shutil.copy(firmware_path, dest_path)
    print(f"Copied {firmware_path} → {dest_path}")

# Run after firmware binary is built
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_firmware)