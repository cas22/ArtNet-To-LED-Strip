import os
import datetime
from SCons.Script import Import

Import("env")


# Generate a timestamped version string
version = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")

# Pass it as a compiler define
env.Append( # type: ignore
    CPPDEFINES=[("VERSION", f'\\"{version}\\"')]
)  

# Determine the destination directory (same as firmware)
project_dir = env.subst("$PROJECT_DIR")
env_name = env["PIOENV"]
destination_dir = os.path.join(project_dir, "builds", env_name)
os.makedirs(destination_dir, exist_ok=True)

# Create version.txt in that folder
version_file = os.path.join(destination_dir, "version.txt")
with open(version_file, "w") as f:
    f.write(version + "\n")

print(f"Created {version_file} with version {version}")